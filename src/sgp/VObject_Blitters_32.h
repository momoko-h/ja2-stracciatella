#ifndef SGP_VOBJECT_BLITTERS_32
#define SGP_VOBJECT_BLITTERS_32

#include "Types.h"
#include "VObject.h"
#include "VSurface.h"
#include <stdint.h>
struct SDL_Texture;


template<typename T>
struct Blitter
{
protected:
	//
	// Blitting parameters generated by ParseArgs
	//
	mutable T *                   DstPtr;
	mutable std::uint8_t const *  SrcPtr;
	mutable int                   LeftSkip;
	mutable int                   RightSkip;
	mutable int                   BottomSkip;
	mutable int                   LineSkip;
	mutable int                   BlitLength;
	mutable int                   BlitHeight;

	SDL_Texture *            texture{ nullptr };

	// Returns false if we can skip blitting because we are
	// outside the clipping rect.
	bool ParseArgs() const;

public:
	//
	// Input arguments
	//
	T *                      buffer;
	int                      pitch;
	std::uint16_t            srcObjectIndex;
	SGPVObject const *       srcVObject;
	int                      x;
	int                      y;
	SGPRect const *          clipregion{ nullptr };

	// Only used by MonoShadow
	T                        Foreground;
	T                        Background;
	T                        Shadow;

	Blitter() = default;
	Blitter(Blitter const&) = delete;
	Blitter & operator=(Blitter const&) = delete;
	Blitter(SGPVSurface *);
	Blitter(SDL_Texture *);
	~Blitter();

	void MonoShadow() const;
	void Transparent() const;
};


// Somewhat fast RGB565 to ABGR8888 (same as RGBA32 on little-endian CPUs) conversion routine.
// Useless in this form on big-endian if we use RGBA32 textures, but easily adapted.
constexpr std::uint32_t ABGR8888(std::uint16_t const RGB565)
{
	constexpr std::uint8_t RB5[32] {0,9,17,25,33,42,50,58,66,75,83,91,99,107,116,124,132,140,149,157,165,173,181,190,198,206,214,223,231,239,247,255};
	constexpr std::uint8_t G6[64]  {0,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73,77,81,85,90,94,98,102,106,110,114,118,122,126,130,134,138,142,146,150,154,158,162,166,170,175,179,183,187,191,195,199,203,207,211,215,219,223,227,231,235,239,243,247,251,255};
	return
		RB5[RGB565 & 0x1f] |               // red
		(G6[(RGB565 >> 5) & 0x3f] << 8) |  // green
		(RB5[(RGB565 >> 11)] << 16) |      // blue
		(0xff << 24);                      // alpha
};

#endif
