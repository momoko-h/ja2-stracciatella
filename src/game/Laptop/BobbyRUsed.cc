#include "Directories.h"
#include "Laptop.h"
#include "BobbyRUsed.h"
#include "BobbyR.h"
#include "BobbyRGuns.h"
#include "VObject.h"
#include "Text.h"
#include "LaptopSave.h"
#include "Button_System.h"
#include "Video.h"
#include "VSurface.h"


void EnterBobbyRUsed()
{
	InitBobbyBrTitle();

	SetFirstLastPagesForUsed();

	//Draw menu bar
	InitBobbyMenuBar( );

	RenderBobbyRUsed( );
}


void ExitBobbyRUsed()
{
	DeleteBobbyMenuBar();
	DeleteBobbyBrTitle();
	DeleteMouseRegionForBigImage();

	giCurrentSubPage = gusCurWeaponIndex;
	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_USED;
}


void RenderBobbyRUsed()
{
  auto const background = SP::AddVideoObjectFromFile(LAPTOPDIR "/usedbackground.sti");
  WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES, BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, background.get());

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/usedgrid.sti", 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	DisplayItemInfo(BOBBYR_USED_ITEMS);

	UpdateButtonText(guiCurrentLaptopMode);
  MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
  InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}
