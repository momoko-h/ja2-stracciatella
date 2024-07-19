#ifndef _GAME_VERSION_H_
#define _GAME_VERSION_H_

#include "Types.h"


//
//	Keeps track of the game version
//

extern const char g_version_label[];
extern const char g_version_number[16];


//
//		Keeps track of the saved game version.  Increment the saved game version whenever
//	you will invalidate the saved game file
//

constexpr UINT32 SAVE_GAME_VERSION = 103;

// Revision history:
// 103: added support for weapons with an item index > 70.
constexpr UINT32 SAVE_GAME_VERSION_MORE_WEAPONS = 103;

#endif
