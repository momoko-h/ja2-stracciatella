#include "Font.h"
#include "FPS.h"
#include "SDL.h"
#include "VObject.h"
#include "VObject_Blitters_32.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <numeric>
#include <vector>
#include <string_theory/format>


using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
    extern SGPVObject* guiBULLSEYE;

namespace FPS
{

struct SDLDeleter
{
	void operator()(SDL_Texture *t) { SDL_DestroyTexture(t); }
};


GameLoopFunc_t  ActualGameLoop; // the real game loop function
GameLoopFunc_t  GameLoopPtr;    // the function MainLoop will call
RenderPresent_t RenderPresentPtr{ SDL_RenderPresent };

std::unique_ptr<SDL_Texture, SDLDeleter> Texture;
SGPVObject * DisplayFont;

std::vector<Clock::duration> LastGameLoopDurations;
Clock::time_point TimeLastDisplayed;

unsigned FramesSinceLastDisplay;


void UpdateTexture(SDL_Renderer * const renderer)
{
	SetFontAttributes(DisplayFont, FONT_FCOLOR_WHITE);

	Texture.reset(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING, 320, 26));
	if (!Texture)
	{
		// Just do nothing, not being able to display FPS is not critical.
		return;
	}
	SDL_SetTextureBlendMode(Texture.get(), SDL_BLENDMODE_BLEND);

	MPrint(Texture.get(), 0, 0, ST::format("{} FPS", FramesSinceLastDisplay));

	if (!LastGameLoopDurations.empty())
	{
		auto const averageLoopDuration =
			std::accumulate(
				LastGameLoopDurations.begin(),
				LastGameLoopDurations.end(),
				Clock::duration{0})
			/ LastGameLoopDurations.size();

		MPrint(Texture.get(), 0, 12, ST::format("Game Loop: {} micros",
			std::chrono::duration_cast<std::chrono::microseconds>
				(averageLoopDuration).count()));
	}

	// TODO: Remove this testing-only code.
	// TODO: Need better source VObject which uses color 254.
	Blitter<uint32_t> blitter{Texture.get()};
	blitter.srcVObject = guiBULLSEYE;
	blitter.srcObjectIndex = 0;
	blitter.y = 1;
	blitter.x = 180; blitter.Transparent();
}


void RenderPresentHook(SDL_Renderer * const renderer)
{
	++FramesSinceLastDisplay;

	auto const now = Clock::now();

	// We must copy the texture each frame otherwise it would flicker,
	// but we only update its content once per second to make it less noisy.
	if (now > TimeLastDisplayed + 1s)
	{
		UpdateTexture(renderer);
		TimeLastDisplayed = now;
		FramesSinceLastDisplay = 0;
		LastGameLoopDurations.clear();
	}

	SDL_Rect const dest{ 11, 23, 320, 26 };
	SDL_RenderCopy(renderer, Texture.get(), nullptr, &dest);
	SDL_RenderPresent(renderer);
}


void GameLoopHook()
{
	auto const before = Clock::now();
	ActualGameLoop();
	auto const elapsed = Clock::now() - before;

	// Only store "interesting" game loops
	if (elapsed > 150us)
	{
		LastGameLoopDurations.push_back(elapsed);
	}
}


void Init(GameLoopFunc_t const gameLoop, SGPVObject * const displayFont)
{
	GameLoopPtr = ActualGameLoop = gameLoop;
	DisplayFont = displayFont;
}


void ToggleOnOff()
{
	if (GameLoopPtr == ActualGameLoop)
	{
		// Currently disabled
		RenderPresentPtr = RenderPresentHook;
		GameLoopPtr = GameLoopHook;
	}
	else
	{
		// Currently enabled
		RenderPresentPtr = SDL_RenderPresent;
		GameLoopPtr = ActualGameLoop;
		Texture.reset();
	}
}

}
