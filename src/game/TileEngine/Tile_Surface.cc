#include <stdexcept>
#include <memory>

#include "HImage.h"
#include "Structure.h"
#include "TileDef.h"
#include "Tile_Surface.h"
#include "VObject.h"
#include "WorldDef.h"
#include "WorldDat.h"
#include "Debug.h"
#include "Smooth.h"
#include "MouseSystem.h"
#include "Sys_Globals.h"
#include "TileDat.h"
#include "FileMan.h"
#include "MemMan.h"
#include "Tile_Cache.h"

#include "ContentManager.h"
#include "GameInstance.h"

#include "slog/slog.h"

TILE_IMAGERY				*gTileSurfaceArray[ NUMBEROFTILETYPES ];


TILE_IMAGERY* LoadTileSurface(const char* cFilename)
try
{
	// Add tile surface
	AutoSGPImage   hImage(CreateImage(cFilename, IMAGE_ALLDATA));
  auto hVObject = SP::AddVideoObjectFromHImage(hImage);

	// Load structure data, if any.
	// Start by hacking the image filename into that for the structure data
  std::string cStructureFilename(FileMan::replaceExtension(cFilename, ".jsd"));

	AutoStructureFileRef pStructureFileRef;
	if (GCM->doesGameResExists( cStructureFilename ))
	{
    SLOGD(DEBUG_TAG_TILES, "loading tile %s", cStructureFilename.c_str());

		pStructureFileRef = LoadStructureFile( cStructureFilename.c_str() );

		if (hVObject->SubregionCount() != pStructureFileRef->usNumberOfStructures)
		{
			throw std::runtime_error("Structure file error");
		}

		AddZStripInfoToVObject(hVObject.get(), pStructureFileRef, FALSE, 0);
	}

  auto pTileSurf = std::make_unique<TILE_IMAGERY>();
  memset(pTileSurf.get(), 0, sizeof(TILE_IMAGERY));

	if (pStructureFileRef && pStructureFileRef->pAuxData != NULL)
	{
		pTileSurf->pAuxData = pStructureFileRef->pAuxData;
		pTileSurf->pTileLocData = pStructureFileRef->pTileLocData;
	}
	else if (hImage->uiAppDataSize == hVObject->SubregionCount() * sizeof(AuxObjectData))
	{
		// Valid auxiliary data, so make a copy of it for TileSurf
		pTileSurf->pAuxData = MALLOCN(AuxObjectData, hVObject->SubregionCount());
		memcpy( pTileSurf->pAuxData, hImage->pAppData, hImage->uiAppDataSize );
	}
	else
	{
		pTileSurf->pAuxData = NULL;
	}

	pTileSurf->vo                = hVObject.release();
	pTileSurf->pStructureFileRef = pStructureFileRef.Release();
	return pTileSurf.release();
}
catch (...)
{
	SET_ERROR("Could not load tile file: %s", cFilename);
	throw;
}


void DeleteTileSurface(TILE_IMAGERY* const pTileSurf)
{
	if ( pTileSurf->pStructureFileRef != NULL )
	{
		FreeStructureFile( pTileSurf->pStructureFileRef );
	}
	else
	{
		// If a structure file exists, it will free the auxdata.
		// Since there is no structure file in this instance, we
		// free it ourselves.
		if (pTileSurf->pAuxData != NULL)
		{
			MemFree( pTileSurf->pAuxData );
		}
	}

	DeleteVideoObject(pTileSurf->vo);
  delete pTileSurf;
}


void SetRaisedObjectFlag(char const* const filename, TILE_IMAGERY* const t)
{
	static char const RaisedObjectFiles[][9] =
	{
		"bones",
		"bones2",
		"grass2",
		"grass3",
		"l_weed3",
		"litter",
		"miniweed",
		"sblast",
		"sweeds",
		"twigs",
		"wing"
	};

	if (DEBRISWOOD != t->fType && t->fType != DEBRISWEEDS && t->fType != DEBRIS2MISC && t->fType != ANOTHERDEBRIS) return;

	// Loop through array of RAISED objecttype imagery and set global value
  std::string rootfile(FileMan::getFileNameWithoutExt(filename));
	for (char const (*i)[9] = RaisedObjectFiles; i != endof(RaisedObjectFiles); ++i)
	{
		if (strcasecmp(*i, rootfile.c_str()) != 0) continue;
		t->bRaisedObjectType = TRUE;
		return;
	}
}
