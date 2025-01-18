#include "HImage.h"
#include "RenderWorld.h"
#include "Shading.h"
#include "VObject.h"
#include "VObject_Blitters.h"
#include "WCheck.h"

namespace {

using pixel_t = UINT16;
using BlitterCore_t = void (*)(pixel_t * dst, UINT8 const * src,
	UINT16 * zdst, UINT16 zval, pixel_t const * palette16);

// These two functions are the extracted boilerplate of most of the old
// blitters. They compute the source, destination and z buffer pointers
// and then loop over the individual rows to blit the pixels using the
// supplied blitter core.

template<BlitterCore_t BlitterCore>
void Unrestricted(pixel_t * const buf, UINT32 const uiDestPitchBYTES,
	UINT16 * const zbuf, UINT16 const zval,
	PCSGPVObject const hSrcVObject,
	int const iX, int const iY, UINT16 const usIndex)
{
	// Get Offsets from Index into structure
	auto const& e    = hSrcVObject->SubregionProperties(usIndex);
	int       height = e.usHeight;
	int const width  = e.usWidth;

	// Add to start position of dest buffer
	int const x = iX + e.sOffsetX;
	int const y = iY + e.sOffsetY;

	// Validations
	CHECKV(x >= 0);
	CHECKV(y >= 0);

	int     const         pitch = static_cast<int>(uiDestPitchBYTES / 2);
	UINT8   const *       src   = hSrcVObject->PixData(e);
	pixel_t       *       dst   = buf + pitch * y + x;
	pixel_t const * const pal   = hSrcVObject->CurrentShade();
	int     const     line_skip = pitch - width;

	do
	{
		for (;;)
		{
			UINT8 data = *src++;

			if (data == 0) break;
			if (data & 0x80)
			{
				data &= 0x7F;
				dst  += data;
			}
			else
			{
				do
				{
					BlitterCore(dst, src, zbuf + (dst - buf), zval, pal);
					++src;
					++dst;
				}
				while (--data > 0);
			}
		}
		dst 	+= line_skip;
	}
	while (--height > 0);
}


template<BlitterCore_t BlitterCore>
void Clipped(pixel_t * const buf, UINT32 const uiDestPitchBYTES,
	UINT16 * const zbuf, UINT16 const zval,
	PCSGPVObject const hSrcVObject, int const iX, int const iY,
	UINT16 const usIndex, PClipRegion clipregion)
{
	/** Difference or Zero */
	auto DoZ = [](int const a, int const b)
	{
		return a > b ? a - b : 0;
	};

	// Get offsets from index into structure.
	ETRLEObject const& e      = hSrcVObject->SubregionProperties(usIndex);
	int const height = e.usHeight;
	int const width  = e.usWidth;

	// Add to start position of dest buffer.
	int const x = iX + e.sOffsetX;
	int const y = iY + e.sOffsetY;

	if (!clipregion) clipregion = &ClippingRect;

	/* Calculate rows hanging off each side of the screen and check if the whole
	 * thing is clipped. */
	int const left_skip   = DoZ(clipregion->iLeft, x);
	if (left_skip   >= width)  return;
	int       top_skip    = DoZ(clipregion->iTop,  y);
	if (top_skip    >= height) return;
	int const right_skip  = DoZ(x + width,  clipregion->iRight);
	if (right_skip  >= width)  return;
	int const bottom_skip = DoZ(y + height, clipregion->iBottom);
	if (bottom_skip >= height) return;

	// Calculate the remaining rows and columns to blit.
	int const blit_length = width  - left_skip - right_skip;
	int       blit_height = height - top_skip  - bottom_skip;

	int     const        pitch     = static_cast<int>(uiDestPitchBYTES / 2);
	UINT8   const*       src       = hSrcVObject->PixData(e);
	pixel_t      *       dst       = buf  + pitch * (y + top_skip) + (x + left_skip);
	pixel_t const* const pal       = hSrcVObject->CurrentShade();
	int     const        line_skip = pitch - blit_length;

	for (; top_skip > 0; --top_skip)
	{
		for (;;)
		{
			auto const px_count = *src++;
			if (px_count & 0x80) continue;
			if (px_count == 0)   break;
			src += px_count;
		}
	}

	do
	{
		int ls_count;
		int px_count;
		for (ls_count = left_skip; ls_count > 0; ls_count -= px_count)
		{
			px_count = *src++;
			if (px_count & 0x80)
			{
				px_count &= 0x7F;
				if (px_count > ls_count)
				{
					px_count -= ls_count;
					ls_count  = blit_length;
					goto BlitTransparent;
				}
			}
			else
			{
				if (px_count > ls_count)
				{
					src      += ls_count;
					px_count -= ls_count;
					ls_count  = blit_length;
					goto BlitNonTransLoop;
				}
				src += px_count;
			}
		}

		ls_count = blit_length;
		while (ls_count > 0)
		{
			px_count = *src++;
			if (px_count & 0x80)
			{ // Skip transparent pixels.
				px_count &= 0x7F;
BlitTransparent:
				if (px_count > ls_count) px_count = ls_count;
				ls_count -= px_count;
				dst      += px_count;
			}
			else
			{ // Blit non-transparent pixels.
BlitNonTransLoop:
				int unblitted = 0;
				if (px_count > ls_count)
				{
					unblitted = px_count - ls_count;
					px_count  = ls_count;
				}
				ls_count -= px_count;

				do
				{
					BlitterCore(dst, src, zbuf + (dst - buf), zval, pal);
					++src;
					++dst;
				}
				while (--px_count > 0);
				src += unblitted;
			}
		}

		while (*src++ != 0) {} // Skip along until we hit and end-of-line marker.
		dst  += line_skip;
	}
	while (--blit_height > 0);
}


void Transparent(pixel_t * dst, UINT8 const * src,
	[[maybe_unused]] UINT16 * zdst, [[maybe_unused]] UINT16 zval,
	pixel_t const * palette16)
{
	*dst = palette16[*src];
}


void Shadow(pixel_t * dst, [[maybe_unused]] UINT8 const * src,
	[[maybe_unused]] UINT16 * zdst, [[maybe_unused]] UINT16 zval,
	[[maybe_unused]] pixel_t const * palette16)
{
	*dst = ShadeTable[*dst];
}


// Helper function objects for blitters implemented using function templates.
using UpdateZOrDont_t = void (*)(UINT16 *, UINT16);

// This version is used by the blitters that do not have 'NB' in their name.
// 'NB' probably means "no bleed through".
void UpdateZ(UINT16 * zdst, UINT16 zval) { *zdst = zval; }

// This version does not update the ZBuffer and needs the following line
// as a somewhat elaborate way to say NOP. Is is used by 'NB' blitters.
void DontUpdateZ(UINT16 *, UINT16) {}


/**********************************************************************************************

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. As it is blitting, it checks the Z value of the ZBuffer, and if the
	pixel's Z level is below that of the current pixel, it is written on, and the Z value is
	updated to the current value,	for any non-transparent pixels. The Z-buffer is 16 bit, and
	must be the same dimensions (including Pitch) as the destination.

	Blits every second pixel ("Translucents").

**********************************************************************************************/
template<UpdateZOrDont_t UpdateZOrDont>
void TransZTranslucent(pixel_t * dst, UINT8 const * src,
	UINT16 * zdst, UINT16 zval,
	pixel_t const * palette16)
{
	if (*zdst < zval)
	{
		UpdateZOrDont(zdst, zval);
		*dst =
			((palette16[*src] >> 1) & guiTranslucentMask) +
			((*dst            >> 1) & guiTranslucentMask);
	}
}


/* Blit an image into the destination buffer, using an ETRLE brush as a source
 * and a 16-bit buffer as a destination. As it is blitting, it checks the Z-
 * value of the ZBuffer, and if the pixel's Z level is below that of the current
 * pixel, it is written on, and the Z value is updated to the current value, for
 * any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
 * dimensions (including Pitch) as the destination. */
template<UpdateZOrDont_t UpdateZOrDont>
void TransZ(pixel_t * dst, UINT8 const * src,
	UINT16 * zdst, UINT16 zval,
	pixel_t const * palette16)
{
	if (*zdst < zval)
	{
		UpdateZOrDont(zdst, zval);
		*dst = palette16[*src];
	}
}


template<UpdateZOrDont_t UpdateZOrDont>
void ShadowZ(pixel_t * dst, [[maybe_unused]] UINT8 const * src,
	UINT16 * zdst, UINT16 zval,
	[[maybe_unused]] pixel_t const * palette16)
{
	if (*zdst < zval)
	{
		UpdateZOrDont(zdst, zval);
		*dst = ShadeTable[*dst];
	}
}
}

// Macro to implement both the clipped and unclipped Z blitters for the given core.
#define DefineZBlitter(fnName, core) \
void fnName (UINT16 * buf, UINT32 uiDestPitchBYTES, UINT16 * zbuf, UINT16 zval, PCSGPVObject hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)\
{\
	Unrestricted<core>(buf, uiDestPitchBYTES, zbuf, zval, hSrcVObject, iX, iY, usIndex);\
}\
\
void fnName## Clip(UINT16 * buf, UINT32 uiDestPitchBYTES, UINT16 * zbuf, UINT16 zval, PCSGPVObject hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, PClipRegion clipregion)\
{\
	Clipped<core>(buf, uiDestPitchBYTES, zbuf, zval, hSrcVObject, iX, iY, usIndex, clipregion);\
}

DefineZBlitter(Blt8BPPDataTo16BPPBufferTransZ,   TransZ<UpdateZ>)
DefineZBlitter(Blt8BPPDataTo16BPPBufferTransZNB, TransZ<DontUpdateZ>)

DefineZBlitter(Blt8BPPDataTo16BPPBufferTransZTranslucent,   TransZTranslucent<UpdateZ>)
DefineZBlitter(Blt8BPPDataTo16BPPBufferTransZNBTranslucent, TransZTranslucent<DontUpdateZ>)

DefineZBlitter(Blt8BPPDataTo16BPPBufferShadowZ,   ShadowZ<UpdateZ>)
DefineZBlitter(Blt8BPPDataTo16BPPBufferShadowZNB, ShadowZ<DontUpdateZ>)

#undef DefineZBlitter

// Macro to implement both the clipped and unclipped non-Z blitters for the given core.
#define DefineBlitter(core) \
void Blt8BPPDataTo16BPPBuffer ##core(UINT16 * buf, UINT32 uiDestPitchBYTES, PCSGPVObject hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)\
{\
	Unrestricted<core>(buf, uiDestPitchBYTES, gpZBuffer, 0, hSrcVObject, iX, iY, usIndex);\
}\
\
void Blt8BPPDataTo16BPPBuffer ##core## Clip(UINT16 * buf, UINT32 uiDestPitchBYTES, PCSGPVObject hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, PClipRegion clipregion)\
{\
	Clipped<core>(buf, uiDestPitchBYTES, gpZBuffer, 0, hSrcVObject, iX, iY, usIndex, clipregion);\
}

DefineBlitter(Transparent)
DefineBlitter(Shadow)
