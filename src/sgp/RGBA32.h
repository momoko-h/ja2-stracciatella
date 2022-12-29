#ifndef RGBA32_H
#define RGBA32_H

#include <SDL_pixels.h>
#include <memory>

struct rgba32 : public SDL_Color
{
	static const std::unique_ptr<SDL_PixelFormat, decltype(&SDL_FreeFormat)> rgb565PixelFormat;

	constexpr rgba32() noexcept
		: SDL_Color{}
	{
		a = 255;
	}

	constexpr rgba32(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) noexcept
		: SDL_Color{}
	{
		r = red; g = green; b = blue; a = alpha;
	}

	Uint16 asRGB565() const noexcept
	{
		return static_cast<Uint16>(SDL_MapRGB(rgb565PixelFormat.get(), r, g, b));
	}
};

#endif
