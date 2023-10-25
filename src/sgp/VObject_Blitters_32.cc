#include "VObject_Blitters_32.h"
#include "Debug.h"
#include "HImage.h"
#include "SDL.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include <cstdint>

#ifdef __GNUC__
// Not getting signed and unsigned mixed up is important for the
// calculations in the blitters, so let the compiler help us out.
// To make this simple and foolproof, we use plain int-type
// variables whenever possible.
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wsign-compare"
#pragma GCC diagnostic warning "-Wold-style-cast"
#endif

static const SGPRect NoClipRect = { 0, 0, UINT16_MAX, UINT16_MAX };

template<typename T>
Blitter<T>::Blitter(SGPVSurface * surface)
{
	auto const& sdl_surface = surface->GetSDLSurface();
	// In SDL 2.0, only surfaces with RLE optimization must be locked.
	// Ensure this is not such a surface.
	if (SDL_MUSTLOCK(&sdl_surface))
	{
		throw 1; // TODO
	}

	buffer = static_cast<T *>(sdl_surface.pixels);
	pitch = sdl_surface.pitch;
}
template Blitter<uint16_t>::Blitter(SGPVSurface * surface);
template Blitter<uint32_t>::Blitter(SGPVSurface * surface);


template<typename T>
Blitter<T>::Blitter(SDL_Texture * texture)
{
	void * pixels;
	if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0)
	{
		throw 1; // TODO
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
bool Blitter<T>::ParseArgs()
{
	// Assertions
	Assert(srcVObject != nullptr);
	Assert(buffer != nullptr);

	// Get Offsets from Index into structure
	auto const& pTrav = srcVObject->SubregionProperties(srcObjectIndex);
	int  const  height = pTrav.usHeight;
	int  const  width  = pTrav.usWidth;

	// Add to start position of dest buffer
	TempX = x + pTrav.sOffsetX;
	TempY = y + pTrav.sOffsetY;

	if (clipregion == nullptr)
	{
		clipregion = &NoClipRect;
	}
	int const ClipX1 = clipregion->iLeft;
	int const ClipY1 = clipregion->iTop;
	int const ClipX2 = clipregion->iRight;
	int const ClipY2 = clipregion->iBottom;

	// Calculate rows hanging off each side of the screen
	LeftSkip    = std::min(ClipX1 - std::min(ClipX1, TempX), width);
	RightSkip   = std::clamp(TempX + width - ClipX2, 0, width);
	int TopSkip = std::min(ClipY1 - std::min(ClipY1, TempY), height);
	BottomSkip  = std::clamp(TempY + height - ClipY2, 0, height);

	// calculate the remaining rows and columns to blit
	BlitLength = width  - LeftSkip - RightSkip;
	BlitHeight = height - TopSkip  - BottomSkip;

	// check if whole thing is clipped
	if ((LeftSkip >= width) || (RightSkip >= height)) return false;

	// check if whole thing is clipped
	if ((TopSkip >= height) || (BottomSkip >= height)) return false;

	int const destSurfaceWidth = pitch / static_cast<int>(sizeof(T));
	DstPtr = buffer + destSurfaceWidth * (TempY + TopSkip) + TempX + LeftSkip;
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
template bool Blitter<uint16_t>::ParseArgs();
template bool Blitter<uint32_t>::ParseArgs();


template<typename T>
void Blitter<T>::MonoShadow()
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
template void Blitter<uint16_t>::MonoShadow();
template void Blitter<uint32_t>::MonoShadow();

#if 0
/**********************************************************************************************
Blt8BPPDataTo16BPPBufferMonoShadowClip

	Uses a bitmap an 8BPP template for blitting. Anywhere a 1 appears in the bitmap, a shadow
	is blitted to the destination (a black pixel). Any other value above zero is considered a
	forground color, and zero is background. If the parameter for the background color is zero,
	transparency is used for the background.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferMonoShadowClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, UINT16 usForeground, UINT16 usBackground, UINT16 usShadow )
{
	UINT32 Unblitted;
	UINT8  *DestPtr;
	UINT32 LineSkip;
	INT32  LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	ETRLEObject const& pTrav = hSrcVObject->SubregionProperties(usIndex);
	UINT32      const  usHeight = pTrav.usHeight;
	UINT32      const  usWidth  = pTrav.usWidth;

	// Add to start position of dest buffer
	INT32 const iTempX = iX + pTrav.sOffsetX;
	INT32 const iTempY = iY + pTrav.sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
	RightSkip = std::clamp(iTempX + usWidth - ClipX2, 0, usWidth);
	TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
	BottomSkip = std::clamp(iTempY + usHeight - ClipY2, 0, usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength=(usWidth-LeftSkip-RightSkip);
	BlitHeight=(usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=usWidth) || (RightSkip >=usWidth))
		return;

	// check if whole thing is clipped
	if((TopSkip >=usHeight) || (BottomSkip >=usHeight))
		return;

	UINT8 const* SrcPtr = hSrcVObject->PixData(pTrav);
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	UINT32 PxCount;

	while (TopSkip > 0)
	{
		for (;;)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80) continue;
			if (PxCount == 0) break;
			SrcPtr += PxCount;
		}
		TopSkip--;
	}

	do
	{
		for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
				PxCount &= 0x7F;
				if (PxCount > static_cast<UINT32>(LSCount))
				{
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitTransparent;
				}
			}
			else
			{
				if (PxCount > static_cast<UINT32>(LSCount))
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
				if (PxCount > static_cast<UINT32>(LSCount)) PxCount = LSCount;
				LSCount -= PxCount;
				if (usBackground == 0)
				{
					DestPtr += 2 * PxCount;
				}
				else
				{
					while (PxCount-- != 0)
					{
						*(UINT16*)DestPtr = usBackground;
						DestPtr += 2;
					}
				}
			}
			else
			{
BlitNonTransLoop: // blit non-transparent pixels
				if (PxCount > static_cast<UINT32>(LSCount))
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
						case 0:  if (usBackground != 0) *(UINT16*)DestPtr = usBackground; break;
						case 1:  if (usShadow != 0)     *(UINT16*)DestPtr = usShadow;     break;
						default:                        *(UINT16*)DestPtr = usForeground; break;
					}
					DestPtr += 2;
				}
				while (--PxCount > 0);
				SrcPtr += Unblitted;
			}
		}

		while (*SrcPtr++ != 0) {} // skip along until we hit and end-of-line marker
		DestPtr += LineSkip;
	}
	while (--BlitHeight > 0);
}
#endif
#if 0
/**********************************************************************************************
Blt8BPPDataTo16BPPBufferTransparentClip

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. Clips the brush.

**********************************************************************************************/
void Blt8BPPDataTo16BPPBufferTransparentClip(UINT16* const pBuffer, const UINT32 uiDestPitchBYTES, const SGPVObject* const hSrcVObject, const INT32 iX, const INT32 iY, const UINT16 usIndex, const SGPRect* const clipregion)
{
	UINT32 Unblitted;
	UINT8  *DestPtr;
	UINT32 LineSkip;
	INT32  LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	ETRLEObject const& pTrav = hSrcVObject->SubregionProperties(usIndex);
	UINT32      const  usHeight = pTrav.usHeight;
	UINT32      const  usWidth  = pTrav.usWidth;

	// Add to start position of dest buffer
	INT32 const iTempX = iX + pTrav.sOffsetX;
	INT32 const iTempY = iY + pTrav.sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip = std::clamp(iTempX + (INT32)usWidth - ClipX2, 0, (INT32)usWidth);
	TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip = std::clamp(iTempY + (INT32)usHeight - ClipY2, 0, (INT32)usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return;

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return;

	UINT8 const* SrcPtr = hSrcVObject->PixData(pTrav);
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	UINT16 const* const p16BPPPalette = hSrcVObject->CurrentShade();
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	UINT32 LSCount;
	UINT32 PxCount;

	while (TopSkip > 0)
	{
		for (;;)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80) continue;
			if (PxCount == 0) break;
			SrcPtr += PxCount;
		}
		TopSkip--;
	}

	do
	{
		for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
				PxCount &= 0x7F;
				if (PxCount > static_cast<UINT32>(LSCount))
				{
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitTransparent;
				}
			}
			else
			{
				if (PxCount > static_cast<UINT32>(LSCount))
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
				if (PxCount > static_cast<UINT32>(LSCount)) PxCount = LSCount;
				LSCount -= PxCount;
				DestPtr += 2 * PxCount;
			}
			else
			{
BlitNonTransLoop: // blit non-transparent pixels
				if (PxCount > static_cast<UINT32>(LSCount))
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
					*(UINT16*)DestPtr = p16BPPPalette[*SrcPtr++];
					DestPtr += 2;
				}
				while (--PxCount > 0);
				SrcPtr += Unblitted;
			}
		}

		while (*SrcPtr++ != 0) {} // skip along until we hit and end-of-line marker
		DestPtr += LineSkip;
	}
	while (--BlitHeight > 0);
}
#endif

template<typename T>
void Blitter<T>::Transparent()
{
	if (!ParseArgs()) return;

#if 0
	do
	{
		for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount)
		{
			PxCount = *SrcPtr++;
			if (PxCount & 0x80)
			{
				PxCount &= 0x7F;
				if (PxCount > static_cast<UINT32>(LSCount))
				{
					PxCount -= LSCount;
					LSCount = BlitLength;
					goto BlitTransparent;
				}
			}
			else
			{
				if (PxCount > static_cast<UINT32>(LSCount))
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
				if (PxCount > static_cast<UINT32>(LSCount)) PxCount = LSCount;
				LSCount -= PxCount;
				DestPtr += 2 * PxCount;
			}
			else
			{
BlitNonTransLoop: // blit non-transparent pixels
				if (PxCount > static_cast<UINT32>(LSCount))
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
					*(UINT16*)DestPtr = p16BPPPalette[*SrcPtr++];
					DestPtr += 2;
				}
				while (--PxCount > 0);
				SrcPtr += Unblitted;
			}
		}

		while (*SrcPtr++ != 0) {} // skip along until we hit and end-of-line marker
		DestPtr += LineSkip;
	}
	while (--BlitHeight > 0);
#endif
}
template void Blitter<uint16_t>::Transparent();
template void Blitter<uint32_t>::Transparent();
