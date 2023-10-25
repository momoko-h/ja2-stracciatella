#include "VObject_Blitters_32.h"
#include "Debug.h"
#include "HImage.h"
#include "SDL.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "Types.h"
#include <cstdint>
#include <stdexcept>

#ifdef __GNUC__
// Not getting signed and unsigned mixed up is important for the
// calculations in the blitters, so let the compiler help us out.
// To make this simple and foolproof, we use plain int-type
// variables whenever possible.
#pragma GCC diagnostic error "-Wconversion"
#pragma GCC diagnostic error "-Wsign-compare"
#pragma GCC diagnostic error "-Wold-style-cast"
#endif


template<typename T>
Blitter<T>::Blitter(SGPVSurface * surface)
{
	auto const& sdl_surface = surface->GetSDLSurface();
	// In SDL 2.0, only surfaces with RLE optimization must be locked.
	// Ensure this is not such a surface.
	if (SDL_MUSTLOCK(&sdl_surface))
	{
		throw std::logic_error("must-lock surfaces not supported");
	}

	buffer = static_cast<T *>(sdl_surface.pixels);
	pitch = sdl_surface.pitch;
}
template Blitter<uint16_t>::Blitter(SGPVSurface * surface);
template Blitter<uint32_t>::Blitter(SGPVSurface * surface);


template<typename T>
Blitter<T>::Blitter(SDL_Texture * texture)
	: texture{texture}
{
	// Note: this locks the entire texture. If you know the exact
	// target rectangle and need maximum speed, do not use this;
	// lock the texture with a rect argument yourself.
	void * pixels;
	if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0)
	{
		throw std::runtime_error("Could not lock texture");
	}
	buffer = static_cast<T *>(pixels);
}
template Blitter<uint16_t>::Blitter(SDL_Texture * surface);
template Blitter<uint32_t>::Blitter(SDL_Texture * surface);


template<typename T>
Blitter<T>::~Blitter()
{
	if (texture)
	{
		SDL_UnlockTexture(texture);
	}
}
template Blitter<uint16_t>::~Blitter();
template Blitter<uint32_t>::~Blitter();


#if 0
// Copy & paste this for any new blitter
template<typename T>
void Blitter<T>::
{
	if (!ParseArgs()) return;
}
template void Blitter<uint16_t>::();
template void Blitter<uint32_t>::();
#endif

template<typename T>
bool Blitter<T>::ParseArgs() const
{
	// Assertions
	Assert(srcVObject != nullptr);
	Assert(buffer != nullptr);

	// Get Offsets from Index into structure
	auto const& pTrav = srcVObject->SubregionProperties(srcObjectIndex);
	int  const  height = pTrav.usHeight;
	int  const  width  = pTrav.usWidth;

	// Add to start position of dest buffer
	int const tempX = x + pTrav.sOffsetX;
	int const tempY = y + pTrav.sOffsetY;

	int const ClipX1 = clipregion ? clipregion->iLeft : 0;
	int const ClipY1 = clipregion ? clipregion->iTop : 0;
	int const ClipX2 = clipregion ? clipregion->iRight : UINT16_MAX;
	int const ClipY2 = clipregion ? clipregion->iBottom : UINT16_MAX;

	// Calculate rows hanging off each side of the screen
	LeftSkip    = std::min(ClipX1 - std::min(ClipX1, tempX), width);
	RightSkip   = std::clamp(tempX + width - ClipX2, 0, width);
	int TopSkip = std::min(ClipY1 - std::min(ClipY1, tempY), height);
	BottomSkip  = std::clamp(tempY + height - ClipY2, 0, height);

	// calculate the remaining rows and columns to blit
	BlitLength = width  - LeftSkip - RightSkip;
	BlitHeight = height - TopSkip  - BottomSkip;

	// check if whole thing is clipped
	if ((LeftSkip >= width) || (RightSkip >= height)) return false;

	// check if whole thing is clipped
	if ((TopSkip >= height) || (BottomSkip >= height)) return false;

	int const destSurfaceWidth = pitch / static_cast<int>(sizeof(T));
	DstPtr = buffer + destSurfaceWidth * (tempY + TopSkip) + tempX + LeftSkip;
	LineSkip = destSurfaceWidth - BlitLength;

	SrcPtr = srcVObject->PixData(pTrav);

	while (TopSkip > 0)
	{
		for (;;)
		{
			int const PxCount = *SrcPtr++;
			if (PxCount & 0x80) continue;
			if (PxCount == 0) break;
			SrcPtr += PxCount;
		}
		TopSkip--;
	}

	return true;
}
template bool Blitter<uint16_t>::ParseArgs() const;
template bool Blitter<uint32_t>::ParseArgs() const;


template<typename T>
void Blitter<T>::MonoShadow() const
{
	if (!ParseArgs()) return;

	int PxCount;
	int LSCount;
	int Unblitted;

	do
	{
		for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
				PxCount &= 0x7F;
				if (PxCount > LSCount)
				{
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitTransparent;
				}
			}
			else
			{
				if (PxCount > LSCount)
				{
					SrcPtr += LSCount;
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitNonTransLoop;
				}
				SrcPtr += PxCount;
			}
		}

		LSCount = BlitLength;
		while (LSCount > 0)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
BlitTransparent: // skip transparent pixels
				PxCount &= 0x7F;
				if (PxCount > LSCount) PxCount = LSCount;
				LSCount -= PxCount;
				if (Background == 0)
				{
					DstPtr += PxCount;
				}
				else
				{
					while (PxCount-- != 0)
					{
						*DstPtr++ = Background;
					}
				}
			}
			else
			{
BlitNonTransLoop: // blit non-transparent pixels
				if (PxCount > LSCount)
				{
					Unblitted = PxCount - LSCount;
					PxCount = LSCount;
				}
				else
				{
					Unblitted = 0;
				}
				LSCount -= PxCount;

				do
				{
					switch (*SrcPtr++)
					{
						case 0:  if (Background != 0) *DstPtr = Background; break;
						case 1:  if (Shadow != 0)     *DstPtr = Shadow;     break;
						default:                      *DstPtr = Foreground; break;
					}
					DstPtr++;
				}
				while (--PxCount > 0);
				SrcPtr += Unblitted;
			}
		}

		while (*SrcPtr++ != 0) {} // skip along until we hit an end-of-line marker
		DstPtr += LineSkip;
	}
	while (--BlitHeight > 0);

}
template void Blitter<uint16_t>::MonoShadow() const;
template void Blitter<uint32_t>::MonoShadow() const;


/**********************************************************************************************
	formerly Blt8BPPDataTo16BPPBufferTransparentClip

	Blits an image into the destination buffer, using an ETRLE brush as a
	source, and a 16 or 32-bit buffer as a destination. Clips the brush.

**********************************************************************************************/
template<typename T>
void Blitter<T>::Transparent() const
{
	if (!ParseArgs()) return;

	/*
		Even in 32-bit blitters we still use the 16-bit palette for now.
	*/
	auto * const p16BPPPalette = srcVObject->CurrentShade();

	int PxCount;
	int LSCount;
	int Unblitted;

	do
	{
		for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
				PxCount &= 0x7F;
				if (PxCount > LSCount)
				{
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitTransparent;
				}
			}
			else
			{
				if (PxCount > LSCount)
				{
					SrcPtr += LSCount;
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitNonTransLoop;
				}
				SrcPtr += PxCount;
			}
		}

		LSCount = BlitLength;
		while (LSCount > 0)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
BlitTransparent: // skip transparent pixels
				PxCount &= 0x7F;
				if (PxCount > LSCount)
				{
					PxCount = LSCount;
				}
				LSCount -= PxCount;
				DstPtr += PxCount;
			}
			else
			{
BlitNonTransLoop: // blit non-transparent pixels
				if (PxCount > LSCount)
				{
					Unblitted = PxCount - LSCount;
					PxCount = LSCount;
				}
				else
				{
					Unblitted = 0;
				}
				LSCount -= PxCount;

				do
				{
					if constexpr (sizeof(T) == 4)
					{
						*DstPtr++ = ABGR8888(p16BPPPalette[*SrcPtr++]);
					}
					else
					{
						*DstPtr++ = p16BPPPalette[*SrcPtr++];
					}
				}
				while (--PxCount > 0);
				SrcPtr += Unblitted;
			}
		}

		while (*SrcPtr++ != 0) {} // skip along until we hit and end-of-line marker
		DstPtr += LineSkip;
	}
	while (--BlitHeight > 0);
}
template void Blitter<uint16_t>::Transparent() const;
template void Blitter<uint32_t>::Transparent() const;


/*
template<typename T>
void Blitter<T>::
{
	if (!ParseArgs()) return;
}
template void Blitter<uint16_t>::();
template void Blitter<uint32_t>::();
*/
