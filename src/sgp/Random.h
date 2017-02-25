#ifndef __RANDOM_
#define __RANDOM_

#include <array>
#include <random>
#include "Types.h"

// Global mersenne twister random number engine. Use freely for things like std:shuffle.
extern std::mt19937 gMT19937;

extern void InitializeRandom(void);
extern UINT32 Random( UINT32 uiRange );

//Chance( 74 ) returns TRUE 74% of the time.  If uiChance >= 100, then it will always return TRUE.
extern BOOLEAN Chance( UINT32 uiChance );

//Returns a pregenerated random number.
//Used to deter Ian's tactic of shoot, miss, restore saved game :)
extern UINT32 PreRandom( UINT32 uiRange );
extern BOOLEAN PreChance( UINT32 uiChance );

//Returns an array containing the numbers 0-7 in random order.
std::array<uint8_t, 8> GetRandomizedDirections(void);

//IMPORTANT:  Changing this define will invalidate the JA2 save.  If this
//						is necessary, please ifdef your own value.
#define MAX_PREGENERATED_NUMS			256
extern UINT32 guiPreRandomIndex;
extern UINT32 guiPreRandomNums[ MAX_PREGENERATED_NUMS ];

#endif
