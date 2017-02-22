#include <algorithm>
#include <random>
#include "Random.h"
#include <stdlib.h>
#include <time.h>

std::mt19937 gMT19937;
UINT32 guiPreRandomIndex = 0;
UINT32 guiPreRandomNums[ MAX_PREGENERATED_NUMS ];

void InitializeRandom(void)
{
	// Seed the random-number generator with a non-deterministic random number
  // so that the numbers will be different every time we run.
  std::random_device rd;
  gMT19937.seed(rd());

  //Pregenerate all of the random numbers.
  std::generate_n(guiPreRandomNums, 256, gMT19937);
	guiPreRandomIndex = 0;
}

// Returns a pseudo-random integer between 0 and uiRange
UINT32 Random(UINT32 uiRange)
{
  if (uiRange == 0) return 0;
  std::uniform_int_distribution<UINT32> dist(0, uiRange - 1);
  return dist(gMT19937);
}

BOOLEAN Chance( UINT32 uiChance )
{
  static std::uniform_int_distribution<UINT32> dist(0, 99);
  return dist(gMT19937) < uiChance;
}

UINT32 PreRandom( UINT32 uiRange )
{
	UINT32 uiNum;
	if( !uiRange )
		return 0;
	//Extract the current pregenerated number
	/* HACK0007 Stop PreRandom always returning 0 or 1
	 * without ensuring an equal distribution, which
	 * would be a rather complex task with pregenerated randoms
	 */
	uiNum = guiPreRandomNums[ guiPreRandomIndex ] % uiRange;
	//Replace the current pregenerated number with a new one.
	guiPreRandomNums[ guiPreRandomIndex ] = gMT19937();

	//Go to the next index.
	guiPreRandomIndex++;
	if( guiPreRandomIndex >= (UINT32)MAX_PREGENERATED_NUMS )
		guiPreRandomIndex = 0;
	return uiNum;
}

BOOLEAN PreChance( UINT32 uiChance )
{
	return PreRandom(100) < uiChance;
}
