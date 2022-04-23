#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include "JA2Types.h"

enum LoadingScreenID
{
	LOADINGSCREEN_NOTHING,
	LOADINGSCREEN_DAYGENERIC,
	LOADINGSCREEN_DAYTOWN1,
	LOADINGSCREEN_DAYTOWN2,
	LOADINGSCREEN_DAYWILD,
	LOADINGSCREEN_DAYTROPICAL,
	LOADINGSCREEN_DAYFOREST,
	LOADINGSCREEN_DAYDESERT,
	LOADINGSCREEN_DAYPALACE,
	LOADINGSCREEN_NIGHTGENERIC,
	LOADINGSCREEN_NIGHTWILD,
	LOADINGSCREEN_NIGHTTOWN1,
	LOADINGSCREEN_NIGHTTOWN2,
	LOADINGSCREEN_NIGHTFOREST,
	LOADINGSCREEN_NIGHTTROPICAL,
	LOADINGSCREEN_NIGHTDESERT,
	LOADINGSCREEN_NIGHTPALACE,
	LOADINGSCREEN_HELI,
	LOADINGSCREEN_BASEMENT,
	LOADINGSCREEN_MINE,
	LOADINGSCREEN_CAVE,
	LOADINGSCREEN_DAYPINE,
	LOADINGSCREEN_NIGHTPINE,
	LOADINGSCREEN_DAYMILITARY,
	LOADINGSCREEN_NIGHTMILITARY,
	LOADINGSCREEN_DAYSAM,
	LOADINGSCREEN_NIGHTSAM,

	NUM_HARDCODED_LOADING_SCREENS
};


// For use by the game loader, before it can possibly know the situation.
extern UINT8 gubLastLoadingScreenID;

// Return the loading screen ID for the specified sector.
UINT8 GetLoadScreenID(const SGPSector& sector);

/* Set up the loadscreen with specified ID, draw it to the FRAME_BUFFER, and
 * refresh the screen with it. */
void DisplayLoadScreenWithID(UINT8);

#endif
