//------------------------------------------------------------------------------
// Cinematics Module
//
//
//	Stolen from Nemesis by Derek Beland.
//	Originally by Derek Beland and Bret Rowden.
//
//------------------------------------------------------------------------------
#include <stdexcept>
extern "C" {
#include "smacker.h"
}
#include "Cinematics.h"
#include "Debug.h"
#include "FileMan.h"
#include "HImage.h"
#include "Intro.h"
#include "Local.h"
#include "Smack_Stub.h"
#include "SoundMan.h"
#include "Types.h"
#include "VSurface.h"
#include "UILayout.h"
#include "Video.h"

#include "ContentManager.h"
#include "GameInstance.h"
#include "slog/slog.h"

struct SMKFLIC
{
	HWFILE    hFileHandle;
	Smack*    SmackerObject;
  CHAR8     SmackerStatus;
  SDL_Surface*    SmackBuffer;
	UINT32    uiFlags;
	UINT32    uiLeft;
	UINT32    uiTop;
};


// SMKFLIC uiFlags
#define SMK_FLIC_OPEN      0x00000001 // Flic is open
#define SMK_FLIC_PLAYING   0x00000002 // Flic is playing
#define SMK_FLIC_AUTOCLOSE 0x00000008 // Close when done

static SMKFLIC SmkList[4];


BOOLEAN SmkPollFlics(void)
{
	BOOLEAN fFlicStatus = FALSE;
	FOR_EACH(SMKFLIC, i, SmkList)
	{
		if (!(i->uiFlags & SMK_FLIC_PLAYING)) continue;
		fFlicStatus = TRUE;

    Smack* const smkobj = i->SmackerObject;

    SmackDoFrame(smkobj, i->uiLeft, i->uiTop);

    if (i->SmackerStatus == SMK_LAST )
		{
      if (i->uiFlags & SMK_FLIC_AUTOCLOSE) SmkCloseFlic(i);
		}
		else
		{
			i->SmackerStatus = SmackNextFrame(smkobj);
		}
	}

	return fFlicStatus;
}


void SmkInitialize(void)
{
	// Wipe the flic list clean
	memset(SmkList, 0, sizeof(SmkList));
}


void SmkShutdown(void)
{
	// Close and deallocate any open flics
	FOR_EACH(SMKFLIC, i, SmkList)
	{
		if (i->uiFlags & SMK_FLIC_OPEN) SmkCloseFlic(i);
	}
}


static SMKFLIC* SmkOpenFlic(const char* filename);


SMKFLIC* SmkPlayFlic(const char* const filename, const UINT32 left, const UINT32 top, const BOOLEAN auto_close)
{
	SMKFLIC* const sf = SmkOpenFlic(filename);
	if (sf == NULL) return NULL;

	// Set the blitting position on the screen
	sf->uiLeft = left;
	sf->uiTop  = top;

	// We're now playing, flag the flic for the poller to update
	sf->uiFlags |= SMK_FLIC_PLAYING;
	if (auto_close) sf->uiFlags |= SMK_FLIC_AUTOCLOSE;

	return sf;
}


static SMKFLIC* SmkGetFreeFlic(void);

static SMKFLIC* SmkOpenFlic(const char* const filename)
try
{
	SMKFLIC* const sf = SmkGetFreeFlic();
	AutoSGPFile file(GCM->openGameResForReading(filename));

	sf->SmackerObject = SmackOpen(file);
	sf->hFileHandle  = file.Release();
	sf->uiFlags     |= SMK_FLIC_OPEN;
	return sf;
}
catch (std::exception &e) {
  SLOGE(DEBUG_TAG_SMK, "Exception during SmkOpenFlic: %s", e.what());
  return NULL;
}


void SmkCloseFlic(SMKFLIC* const sf)
{
  SmackClose(sf->SmackerObject);
  FileClose(sf->hFileHandle);
  memset(sf, 0, sizeof(*sf));
}


static SMKFLIC* SmkGetFreeFlic(void)
{
	FOR_EACH(SMKFLIC, i, SmkList)
	{
		if (!(i->uiFlags & SMK_FLIC_OPEN)) return i;
	}
  throw std::logic_error("SmkGetFreeFlic out of flic slots");
}
