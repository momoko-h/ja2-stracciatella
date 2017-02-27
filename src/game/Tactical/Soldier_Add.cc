#include <algorithm>
#include <functional>
#include <random>
#include <vector>
#include "Soldier_Control.h"
#include "Overhead.h"
#include "Overhead_Types.h"
#include "Isometric_Utils.h"
#include "Interface_Panels.h"
#include "Soldier_Macros.h"
#include "StrategicMap.h"
#include "Strategic.h"
#include "Animation_Control.h"
#include "Soldier_Create.h"
#include "Soldier_Init_List.h"
#include "Soldier_Add.h"
#include "Map_Information.h"
#include "FOV.h"
#include "PathAI.h"
#include "Random.h"
#include "Render_Fun.h"
#include "Meanwhile.h"
#include "Exit_Grids.h"
#include "Debug.h"
#include "Structure.h"

#include "Soldier.h"

// SO, STEPS IN CREATING A MERC!

// 1 ) Setup the SOLDIERCREATE_STRUCT
//			Among other things, this struct needs a sSectorX, sSectorY, and a valid InsertionDirection
//			and InsertionGridNo.
//			This GridNo will be determined by a prevouis function that will examine the sector
//			Infomration regarding placement positions and pick one
// 2 ) Call TacticalCreateSoldier() which will create our soldier
//			What it does is:	Creates a soldier in the Menptr[] array.
//												Allocates the Animation cache for this merc
//												Loads up the intial aniamtion file
//												Creates initial palettes, etc
//												And other cool things.
//			Now we have an allocated soldier, we just need to set him in the world!
// 3) When we want them in the world, call AddSoldierToSector().
//			This function sets the graphic in the world, lighting effects, etc
//			It also formally adds it to the TacticalSoldier slot and interface panel slot.



static std::vector<GridNo> GenerateGridNosInRadiusAroundSweetSpot(
    GridNo const sweetSpot, int const radius, bool const includeSweetSpot = true)
{
  std::vector<GridNo> result;

  int const sweetSpotRow = sweetSpot / WORLD_COLS;
  int const sweetSpotCol = sweetSpot % WORLD_COLS;

  for (int row = std::max(0, sweetSpotRow - radius); row <= std::min(WORLD_ROWS, sweetSpotRow + radius); ++row) {
    for (int col = std::max(0, sweetSpotCol - radius); col <= std::min(WORLD_COLS, sweetSpotCol + radius); ++col) {
      GridNo gridNo = row * WORLD_COLS + col;
      if (includeSweetSpot || (gridNo != sweetSpot)) result.push_back(gridNo);
    }
  }

  return result;
}



//Save AI pathing vars.  changing the distlimit restricts how
//far away the pathing will consider.
struct ScopedSavePathingVars {
  uint8_t NPCAPBudget;
  uint8_t NPCDistLimit;

  ScopedSavePathingVars(uint8_t radius) : NPCAPBudget(gubNPCAPBudget), NPCDistLimit(gubNPCDistLimit) {
    gubNPCAPBudget = 0;
    gubNPCDistLimit = radius;
  }

  ~ScopedSavePathingVars() {
    gubNPCAPBudget = NPCAPBudget;
    gubNPCDistLimit = NPCDistLimit;
  }
};



//clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
//in the square region.
static void ClearReachableFlagForGridNos(std::vector<GridNo> const &gridNos) {
  for (auto const g : gridNos) {
    gpWorldLevelData[g].uiFlags &= (~MAPELEMENT_REACHABLE);
  }
}


// Filter out all grids that are suitable destination
// for this soldier.
static void FilterNotOkGrids(std::vector<GridNo> &gridNos, const SOLDIERTYPE *const pSoldier) {
  gridNos.erase(std::remove_if(gridNos.begin(), gridNos.end(),
    [pSoldier] (GridNo gridNo) {
      return !NewOKDestination(pSoldier, gridNo, TRUE, pSoldier->bLevel);
    }), gridNos.end());
}


// Filter out all grids that are either not reachable or not a suitable destination
// for this soldier.
static void FilterUnReachableOrNotOkGrids(std::vector<GridNo> &gridNos, const SOLDIERTYPE *const pSoldier) {
  gridNos.erase(std::remove_if(gridNos.begin(), gridNos.end(),
    [pSoldier] (GridNo gridNo) {
      return !((gpWorldLevelData[gridNo].uiFlags & MAPELEMENT_REACHABLE) &&
               NewOKDestination(pSoldier, gridNo, TRUE, pSoldier->bLevel));
    }), gridNos.end());
}



static GridNo FindMinimumDistanceGridNoWithCompareFn(
      std::vector<GridNo> const &gridNos,
      GridNo const sweetGridNo,
      decltype(CardinalSpacesAway) const compareFn) {
  int32_t smallestSoFar = INT32_MAX;
  GridNo result = NOWHERE;
  for (auto const gn : gridNos) {
    int32_t v = compareFn(sweetGridNo, gn);
    if (v < smallestSoFar) {
      result = gn;
      smallestSoFar = v;
    }
  }
  return result;
}



//Kris:  modified to actually path from sweetspot to gridno.  Previously, it only checked if the
//destination was sittable (though it was possible that that location would be trapped.
UINT16 FindGridNoFromSweetSpot(const SOLDIERTYPE* const pSoldier, const INT16 sSweetGridNo, const INT8 ubRadius)
{
  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius);
  ScopedSavePathingVars savePathingVars(ubRadius);
  ClearReachableFlagForGridNos(gridNos);

	//Now, find out which of these gridnos are reachable
	//(use the fake soldier and the pathing settings)
  FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE), sSweetGridNo);
  FilterUnReachableOrNotOkGrids(gridNos, pSoldier);

  return FindMinimumDistanceGridNoWithCompareFn(gridNos, sSweetGridNo, CardinalSpacesAway);
}


UINT16 FindGridNoFromSweetSpotThroughPeople(const SOLDIERTYPE* const pSoldier, const INT16 sSweetGridNo, const INT8 ubRadius)
{
  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius);
  ScopedSavePathingVars savePathingVars(ubRadius);
  ClearReachableFlagForGridNos(gridNos);

	//Now, find out which of these gridnos are reachable
	//(use the fake soldier and the pathing settings)
	FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, ( PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE ), sSweetGridNo, pSoldier->bTeam);
  FilterUnReachableOrNotOkGrids(gridNos, pSoldier);

  return FindMinimumDistanceGridNoWithCompareFn(gridNos, sSweetGridNo, GetRangeInCellCoordsFromGridNoDiff);
}


//Kris:  modified to actually path from sweetspot to gridno.  Previously, it only checked if the
//destination was sittable (though it was possible that that location would be trapped.
UINT16 FindGridNoFromSweetSpotWithStructData( SOLDIERTYPE *pSoldier, UINT16 usAnimState, INT16 sSweetGridNo, INT8 ubRadius, UINT8 *pubDirection, BOOLEAN fClosestToMerc )
{
  // If we are already at this gridno....
  if ( pSoldier->sGridNo == sSweetGridNo && !( pSoldier->uiStatusFlags & SOLDIER_VEHICLE ) )
  {
    *pubDirection = pSoldier->bDirection;
    return( sSweetGridNo );
  }

  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius);
  ScopedSavePathingVars savePathingVars(ubRadius);
  ClearReachableFlagForGridNos(gridNos);

	//Now, find out which of these gridnos are reachable
	//(use the fake soldier and the pathing settings)
  FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, ( PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE ), sSweetGridNo);
  FilterUnReachableOrNotOkGrids(gridNos, pSoldier);

  uint16_t usOKToAddStructID = (pSoldier->pLevelNode && pSoldier->pLevelNode->pStructureData)
                             ? pSoldier->pLevelNode->pStructureData->usStructureID
                             : INVALID_STRUCTURE_ID;
  // Get animation surface...
  uint16_t usAnimSurface = DetermineSoldierAnimationSurface( pSoldier, usAnimState );
  // Get structure ref...
  const STRUCTURE_FILE_REF* const pStructureFileRef = GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);
  Assert(pStructureFileRef);

  GridNo sLowestGridNo = NOWHERE;
  int32_t uiLowestRange = INT32_MAX;
  UINT8 ubBestDirection = 0;

  std::function<int32_t(GridNo)> compareFn;
  if (fClosestToMerc) {
    compareFn = [pSoldier] (GridNo sGridNo) {
      int32_t result = FindBestPath( pSoldier, sGridNo, pSoldier->bLevel, pSoldier->usUIMovementMode, NO_COPYROUTE, 0 );
      return result == 0 ? 999 : result;
    };
  } else {
    compareFn = std::bind(GetRangeInCellCoordsFromGridNoDiff, sSweetGridNo, std::placeholders::_1);
  }

  for (auto const sGridNo : gridNos) {
    // Check each struct in each direction
    for (uint8_t cnt3 = 0; cnt3 < 8; ++cnt3)
    {
      if (OkayToAddStructureToWorld(sGridNo, pSoldier->bLevel, &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)], usOKToAddStructID))
      {
        int32_t uiRange = compareFn(sGridNo);

        if ( uiRange < uiLowestRange )
        {
          ubBestDirection = cnt3;
          sLowestGridNo = sGridNo;
          uiLowestRange = uiRange;
        }

        break;
      }
    }
  }

  *pubDirection = ubBestDirection;
  return sLowestGridNo;
}


static UINT16 FindGridNoFromSweetSpotWithStructDataUsingGivenDirectionFirst(SOLDIERTYPE* pSoldier, UINT16 usAnimState, INT16 sSweetGridNo, INT8 ubRadius, UINT8* pubDirection, BOOLEAN fClosestToMerc, INT8 bGivenDirection)
{
  // If we are already at this gridno....
  if ( pSoldier->sGridNo == sSweetGridNo && !( pSoldier->uiStatusFlags & SOLDIER_VEHICLE ) )
  {
    *pubDirection = pSoldier->bDirection;
    return( sSweetGridNo );
  }

  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius);
  ScopedSavePathingVars savePathingVars(ubRadius);
  ClearReachableFlagForGridNos(gridNos);

	//Now, find out which of these gridnos are reachable
	//(use the fake soldier and the pathing settings)
  FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, ( PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE ), sSweetGridNo);
  FilterUnReachableOrNotOkGrids(gridNos, pSoldier);

	int32_t uiLowestRange = 999999;
  uint8_t ubBestDirection;
  GridNo sLowestGridNo = NOWHERE;
          bool fFound = false;

  uint16_t usOKToAddStructID = (pSoldier->pLevelNode && pSoldier->pLevelNode->pStructureData)
                             ? pSoldier->pLevelNode->pStructureData->usStructureID
                             : INVALID_STRUCTURE_ID;
  // Get animation surface...
  uint16_t usAnimSurface = DetermineSoldierAnimationSurface( pSoldier, usAnimState );
  // Get structure ref...
  const STRUCTURE_FILE_REF* const pStructureFileRef = GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);
  Assert(pStructureFileRef);

  std::function<int32_t(GridNo)> compareFn;
  if (fClosestToMerc) {
    compareFn = [pSoldier] (GridNo sGridNo) {
      int32_t result = FindBestPath( pSoldier, sGridNo, pSoldier->bLevel, pSoldier->usUIMovementMode, NO_COPYROUTE, 0 );
      return result == 0 ? 999 : result;
    };
  } else {
    compareFn = std::bind(GetRangeInCellCoordsFromGridNoDiff, sSweetGridNo, std::placeholders::_1);
  }

  for (auto sGridNo : gridNos) 
			{
					BOOLEAN fDirectionFound = FALSE;
					UINT16	usOKToAddStructID;
					UINT16							 usAnimSurface;
          int8_t cnt3;
          int32_t uiRange;

          // OK, check the perfered given direction first
					if (OkayToAddStructureToWorld(sGridNo, pSoldier->bLevel, &pStructureFileRef->pDBStructureRef[OneCDirection(bGivenDirection)], usOKToAddStructID))
					{
						fDirectionFound = TRUE;
            cnt3 = bGivenDirection;
					}
          else
          {
					  // Check each struct in each direction
					  for( cnt3 = 0; cnt3 < 8; cnt3++ )
					  {
              if ( cnt3 != bGivenDirection )
              {
						    if (OkayToAddStructureToWorld(sGridNo, pSoldier->bLevel, &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)], usOKToAddStructID))
						    {
							    fDirectionFound = TRUE;
							    break;
						    }
              }
					  }
          }

					if ( fDirectionFound )
					{
            uiRange = compareFn(sGridNo);

						if ( uiRange < uiLowestRange )
						{
							ubBestDirection = (UINT8)cnt3;
							sLowestGridNo = sGridNo;
							uiLowestRange = uiRange;
							fFound = TRUE;
						}
					}
				
			}
		
	
	if ( fFound )
	{
		// Set direction we chose...
		*pubDirection = ubBestDirection;

		return( sLowestGridNo );
	}
	else
	{
		return( NOWHERE );
	}
}


UINT16 FindGridNoFromSweetSpotWithStructDataFromSoldier(const SOLDIERTYPE* const pSoldier, const UINT16 usAnimState, const INT8 ubRadius, const BOOLEAN fClosestToMerc, const SOLDIERTYPE* const pSrcSoldier)
{
	INT16  sTop, sBottom;
	INT16  sLeft, sRight;
	INT16  cnt1, cnt2, cnt3;
	INT16		sGridNo;
	INT32		uiRange, uiLowestRange = 999999;
	INT32					leftmost;
	INT16 sSweetGridNo;

	sSweetGridNo = pSrcSoldier->sGridNo;


	//Save AI pathing vars.  changing the distlimit restricts how
	//far away the pathing will consider.
  ScopedSavePathingVars savePathingVars(ubRadius);

	sTop		= ubRadius;
	sBottom = -ubRadius;
	sLeft   = - ubRadius;
	sRight  = ubRadius;

	//clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
	//in the square region.
	// ATE: CHECK FOR BOUNDARIES!!!!!!
	for( cnt1 = sBottom; cnt1 <= sTop; cnt1++ )
	{
		leftmost = ( ( sSweetGridNo + ( WORLD_COLS * cnt1 ) )/ WORLD_COLS ) * WORLD_COLS;

		for( cnt2 = sLeft; cnt2 <= sRight; cnt2++ )
		{
			sGridNo = sSweetGridNo + ( WORLD_COLS * cnt1 ) + cnt2;
			if( sGridNo >=0 && sGridNo < WORLD_MAX && sGridNo >= leftmost && sGridNo < ( leftmost + WORLD_COLS ) )
			{
				gpWorldLevelData[ sGridNo ].uiFlags &= (~MAPELEMENT_REACHABLE);
			}
		}
	}

	//Now, find out which of these gridnos are reachable
  FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, ( PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE ), sSweetGridNo);

	uiLowestRange = 999999;

	INT16 sLowestGridNo = NOWHERE;
	for( cnt1 = sBottom; cnt1 <= sTop; cnt1++ )
	{
		leftmost = ( ( sSweetGridNo + ( WORLD_COLS * cnt1 ) )/ WORLD_COLS ) * WORLD_COLS;

		for( cnt2 = sLeft; cnt2 <= sRight; cnt2++ )
		{
			sGridNo = sSweetGridNo + ( WORLD_COLS * cnt1 ) + cnt2;
			if( sGridNo >=0 && sGridNo < WORLD_MAX && sGridNo >= leftmost && sGridNo < ( leftmost + WORLD_COLS )
				&& gpWorldLevelData[ sGridNo ].uiFlags & MAPELEMENT_REACHABLE )
			{
				// Go on sweet stop
				if ( NewOKDestination( pSoldier, sGridNo, TRUE, pSoldier->bLevel ) )
				{
					BOOLEAN fDirectionFound = FALSE;
					UINT16	usOKToAddStructID;
					UINT16							 usAnimSurface;

					if ( fClosestToMerc != 3 )
					{
						if ( pSoldier->pLevelNode != NULL && pSoldier->pLevelNode->pStructureData != NULL )
						{
							usOKToAddStructID = pSoldier->pLevelNode->pStructureData->usStructureID;
						}
						else
						{
							usOKToAddStructID = INVALID_STRUCTURE_ID;
						}

						// Get animation surface...
			 			usAnimSurface = DetermineSoldierAnimationSurface( pSoldier, usAnimState );
						// Get structure ref...
						const STRUCTURE_FILE_REF* const pStructureFileRef = GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);

						// Check each struct in each direction
						for( cnt3 = 0; cnt3 < 8; cnt3++ )
						{
							if (OkayToAddStructureToWorld(sGridNo, pSoldier->bLevel, &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)], usOKToAddStructID))
							{
								fDirectionFound = TRUE;
								break;
							}

						}
					}
					else
					{
						fDirectionFound = TRUE;
						cnt3 = (UINT8)Random( 8 );
					}

					if ( fDirectionFound )
					{
						if ( fClosestToMerc == 1 )
						{
							uiRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );
						}
						else if ( fClosestToMerc == 2 )
						{
							uiRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo ) + GetRangeInCellCoordsFromGridNoDiff( sSweetGridNo, sGridNo );
						}
						else
						{
							//uiRange = GetRangeInCellCoordsFromGridNoDiff( sSweetGridNo, sGridNo );
							uiRange = ABS((sSweetGridNo / MAXCOL) - (sGridNo / MAXCOL)) +
								ABS((sSweetGridNo % MAXROW) - (sGridNo % MAXROW));
						}

						if ( uiRange < uiLowestRange || (uiRange == uiLowestRange && PythSpacesAway( pSoldier->sGridNo, sGridNo ) < PythSpacesAway( pSoldier->sGridNo, sLowestGridNo ) ) )
						{
							sLowestGridNo		= sGridNo;
							uiLowestRange		= uiRange;
						}
					}
				}
			}
		}
	}
	return sLowestGridNo;
}


UINT16 FindGridNoFromSweetSpotExcludingSweetSpot(const SOLDIERTYPE* const pSoldier, const INT16 sSweetGridNo, const INT8 ubRadius)
{
  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius, false);
  FilterNotOkGrids(gridNos, pSoldier);

  return FindMinimumDistanceGridNoWithCompareFn(gridNos, sSweetGridNo, GetRangeInCellCoordsFromGridNoDiff);
}


// Specific function just for the helidrop animation code 467 (see Soldier_Ani.cc)
GridNo FindGridNoFromSweetSpotExcludingSweetSpotInSEQuadrant(const SOLDIERTYPE* const pSoldier)
{
  std::vector<GridNo> gridNos;
  GridNo const sSweetGridNo = pSoldier->sGridNo;
  constexpr uint8_t radius = 3;

  int const sweetSpotRow = sSweetGridNo / WORLD_COLS;
  int const sweetSpotCol = sSweetGridNo % WORLD_COLS;

  for (int row = sweetSpotRow; row <= std::min(WORLD_ROWS, sweetSpotRow + radius); ++row) {
    for (int col = sweetSpotCol; col <= std::min(WORLD_COLS, sweetSpotCol + radius); ++col) {
      GridNo gridNo = row * WORLD_COLS + col;
      if (gridNo != sSweetGridNo) gridNos.push_back(gridNo);
    }
  }

  FilterNotOkGrids(gridNos, pSoldier);
  return FindMinimumDistanceGridNoWithCompareFn(gridNos, sSweetGridNo, GetRangeInCellCoordsFromGridNoDiff);
}


BOOLEAN CanSoldierReachGridNoInGivenTileLimit( SOLDIERTYPE *pSoldier, INT16 sGridNo, INT16 sMaxTiles, INT8 bLevel )
{
	INT32 iNumTiles;
	INT16	sActionGridNo;

	if ( pSoldier->bLevel != bLevel )
	{
		return( FALSE );
	}

	sActionGridNo = FindAdjacentGridEx(pSoldier, sGridNo, NULL, NULL, FALSE, FALSE);

	if ( sActionGridNo == -1 )
	{
		sActionGridNo = sGridNo;
	}

	if ( sActionGridNo == pSoldier->sGridNo )
	{
		return( TRUE );
	}

	iNumTiles = FindBestPath( pSoldier, sActionGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE, PATH_IGNORE_PERSON_AT_DEST );

	if ( iNumTiles <= sMaxTiles && iNumTiles != 0 )
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}


UINT16 FindRandomGridNoFromSweetSpot(const SOLDIERTYPE* const pSoldier, const INT16 sSweetGridNo, const INT8 ubRadius)
{
  auto gridNos = GenerateGridNosInRadiusAroundSweetSpot(sSweetGridNo, ubRadius);
  ScopedSavePathingVars savePathingVars(ubRadius);
  ClearReachableFlagForGridNos(gridNos);

	//Now, find out which of these gridnos are reachable
	//(use the fake soldier and the pathing settings)
  FindBestPathWithDummy(NOWHERE, 0, WALKING, COPYREACHABLE, (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE), sSweetGridNo);
  FilterUnReachableOrNotOkGrids(gridNos, pSoldier);

  std::shuffle(gridNos.begin(), gridNos.end(), gMT19937);

  for (auto gridNo : gridNos) {
    // Don't place crows inside rooms.
    if (pSoldier->ubBodyType != CROW || GetRoom(gridNo) == NO_ROOM)
      return gridNo;
  }

  return NOWHERE;
}


static void AddSoldierToSectorGridNo(SOLDIERTYPE* pSoldier, INT16 sGridNo, UINT8 ubDirection, BOOLEAN fUseAnimation, UINT16 usAnimState, UINT16 usAnimCode);


static void InternalAddSoldierToSector(SOLDIERTYPE* const s, BOOLEAN calculate_direction, BOOLEAN const use_animation, UINT16 const anim_state, UINT16 const anim_code)
{
	if (!s->bActive) return;

	// ATE: Make sure life of elliot is OK if from a meanwhile
	if (AreInMeanwhile() && s->ubProfile == ELLIOT && s->bLife < OKLIFE)
	{
		s->bLife = 25;
	}

	// ADD SOLDIER TO SLOT!
	if (s->uiStatusFlags & SOLDIER_OFF_MAP)
	{
		AddAwaySlot(s);
		// Guy is NOT "in sector"
		s->bInSector = FALSE;
	}
	else
	{
		AddMercSlot(s);
		// Add guy to sector flag
		s->bInSector = TRUE;
	}

	// If a driver or passenger - stop here!
	if (s->uiStatusFlags & SOLDIER_DRIVER)    return;
	if (s->uiStatusFlags & SOLDIER_PASSENGER) return;

	CheckForAndAddMercToTeamPanel(s);

	s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_SPOTTING_CREATURE_ATTACK;
	s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_SMELLED_CREATURE;
	s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_WORRIED_ABOUT_CREATURES;

	INT16 gridno;
	UINT8 direction;
	UINT8 calculated_direction;
	if (s->bTeam == CREATURE_TEAM)
	{
		gridno    = FindGridNoFromSweetSpotWithStructData(s, STANDING, s->sInsertionGridNo, 7, &calculated_direction, FALSE);
		direction = calculate_direction ? calculated_direction : s->ubInsertionDirection;
	}
	else
	{
		if (s->sInsertionGridNo == NOWHERE)
		{ //Add the soldier to the respective entrypoint.  This is an error condition.
		}

		if (s->uiStatusFlags & SOLDIER_VEHICLE)
		{
			gridno = FindGridNoFromSweetSpotWithStructDataUsingGivenDirectionFirst(s, STANDING, s->sInsertionGridNo, 12, &calculated_direction, FALSE, s->ubInsertionDirection);
			// ATE: Override insertion direction
			s->ubInsertionDirection = calculated_direction;
		}
		else
		{
			gridno = FindGridNoFromSweetSpot(s, s->sInsertionGridNo, 7);
			if (gridno == NOWHERE)
			{ // ATE: Error condition - if nowhere use insertion gridno!
				// FIXME: calculate_direction is left uninitialized
				gridno = s->sInsertionGridNo;
			}
			else
			{
				calculated_direction = GetDirectionToGridNoFromGridNo(gridno, CENTER_GRIDNO);
			}
		}

		// Override calculated direction if we were told to....
		if (s->ubInsertionDirection > 100)
		{
			s->ubInsertionDirection -= 100;
			calculate_direction      = FALSE;
		}

		if (calculate_direction)
		{
			direction = calculated_direction;

			// Check if we need to get direction from exit grid
			if (s->bUseExitGridForReentryDirection)
			{
				s->bUseExitGridForReentryDirection = FALSE;

				// OK, we know there must be an exit gridno SOMEWHERE close
				INT16 const sExitGridNo = FindClosestExitGrid(s, gridno, 10);
				if (sExitGridNo != NOWHERE)
				{
					// We found one, calculate direction
					direction = (UINT8)GetDirectionToGridNoFromGridNo(sExitGridNo, gridno);
				}
			}
		}
		else
		{
			direction = s->ubInsertionDirection;
		}
	}

	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) direction = s->bDirection;
	AddSoldierToSectorGridNo(s, gridno, direction, use_animation, anim_state, anim_code);

	CheckForPotentialAddToBattleIncrement(s);
}


void AddSoldierToSector(SOLDIERTYPE* const s)
{
	InternalAddSoldierToSector(s, TRUE, FALSE, 0 , 0);
}


void AddSoldierToSectorNoCalculateDirection(SOLDIERTYPE* const s)
{
	InternalAddSoldierToSector(s, FALSE, FALSE, 0, 0);
}


void AddSoldierToSectorNoCalculateDirectionUseAnimation(SOLDIERTYPE* const s, UINT16 const usAnimState, UINT16 const usAnimCode)
{
	InternalAddSoldierToSector(s, FALSE, TRUE, usAnimState, usAnimCode);
}


static void PlaceSoldierNearSweetSpot(SOLDIERTYPE* const s, const UINT16 anim, const GridNo sweet_spot)
{
	// OK, look for suitable placement....
	UINT8	new_direction;
	const GridNo good_pos = FindGridNoFromSweetSpotWithStructData(s, anim, sweet_spot, 5, &new_direction, FALSE);
	EVENT_SetSoldierPosition(s, good_pos, SSP_NONE);
	EVENT_SetSoldierDirection(s, new_direction);
	EVENT_SetSoldierDesiredDirection(s, new_direction);
}


static void InternalSoldierInSectorSleep(SOLDIERTYPE* const s, INT16 const gridno)
{
	if (!s->bInSector) return;
	UINT16 const anim = AM_AN_EPC(s) ? STANDING : SLEEPING;
	PlaceSoldierNearSweetSpot(s, anim, gridno);
	EVENT_InitNewSoldierAnim( s, anim, 1, TRUE);
}


static void SoldierInSectorIncompaciated(SOLDIERTYPE* const s, INT16 const gridno)
{
	if (!s->bInSector) return;
	PlaceSoldierNearSweetSpot(s, STAND_FALLFORWARD_STOP, gridno);
	EVENT_InitNewSoldierAnim( s, STAND_FALLFORWARD_STOP, 1, TRUE);
}


static void SoldierInSectorAnim(SOLDIERTYPE* const s, INT16 const gridno, UINT16 anim_state)
{
	if (!s->bInSector) return;
	PlaceSoldierNearSweetSpot(s, anim_state, gridno);
	if (!IS_MERC_BODY_TYPE(s)) anim_state = STANDING;
	EVENT_InitNewSoldierAnim(s, anim_state, 1, TRUE);
}


void SoldierInSectorPatient(SOLDIERTYPE* const s, INT16 const gridno)
{
	SoldierInSectorAnim(s, gridno, BEING_PATIENT);
}


void SoldierInSectorDoctor(SOLDIERTYPE* const s, INT16 const gridno)
{
	SoldierInSectorAnim(s, gridno, BEING_DOCTOR);
}


void SoldierInSectorRepair(SOLDIERTYPE* const s, INT16 const gridno)
{
	SoldierInSectorAnim(s, gridno, BEING_REPAIRMAN);
}


static void AddSoldierToSectorGridNo(SOLDIERTYPE* const s, INT16 const sGridNo, UINT8 const ubDirection, BOOLEAN const fUseAnimation, UINT16 const usAnimState, UINT16 const usAnimCode)
{
	// Add merc to gridno

  SoldierSP soldier = GetSoldier(s);

	// Set reserved location!
	s->sReservedMovementGridNo = NOWHERE;

	// Save OLD insertion code.. as this can change...
	UINT8 const insertion_code = s->ubStrategicInsertionCode;

  soldier->removePendingAnimation();

	//If we are not loading a saved game
	SetSoldierPosFlags set_pos_flags = SSP_NONE;
	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME)
	{
		// Set final dest to be the same...
		set_pos_flags = SSP_NO_DEST | SSP_NO_FINAL_DEST;
	}

	// If this is a special insertion location, get path!
	if (insertion_code == INSERTION_CODE_ARRIVING_GAME)
	{
		EVENT_SetSoldierPosition(        s, sGridNo, set_pos_flags);
		EVENT_SetSoldierDirection(       s, ubDirection);
		EVENT_SetSoldierDesiredDirection(s, ubDirection);
	}
	else if (insertion_code != INSERTION_CODE_CHOPPER)
	{
		EVENT_SetSoldierPosition(s, sGridNo, set_pos_flags);

		// if we are loading, dont set the direction (they are already set)
		if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME))
		{
			EVENT_SetSoldierDirection(       s, ubDirection);
			EVENT_SetSoldierDesiredDirection(s, ubDirection);
		}
	}

	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

	if (!(s->uiStatusFlags & SOLDIER_DEAD) && s->bTeam == OUR_TEAM)
	{
		RevealRoofsAndItems(s, FALSE);

		// ATE: Patch fix: If we are in an non-interruptable animation, stop!
		if (s->usAnimState == HOPFENCE)
		{
			s->fInNonintAnim = FALSE;
			SoldierGotoStationaryStance(s);
		}

		EVENT_StopMerc(s, sGridNo, ubDirection);
	}

	// If just arriving, set a destination to walk into from!
	if (insertion_code == INSERTION_CODE_ARRIVING_GAME)
	{
		// Find a sweetspot near...
		INT16 const new_gridno = FindGridNoFromSweetSpot(s, gMapInformation.sNorthGridNo, 4);
		EVENT_GetNewSoldierPath(s, new_gridno, WALKING);
	}

	// If he's an enemy... set presence
	// ATE: Added if not bloodcats, only do this once they are seen
	if (!s->bNeutral && s->bSide != OUR_TEAM && s->ubBodyType != BLOODCAT)
	{
		SetEnemyPresence();
	}

	if (s->uiStatusFlags & SOLDIER_DEAD) return;

	// ATE: Double check if we are on the roof that there is a roof there!
	if (s->bLevel == 1 && !FindStructure(s->sGridNo, STRUCTURE_ROOF))
	{
		SetSoldierHeight(s, 0.0);
	}

	if (insertion_code == INSERTION_CODE_ARRIVING_GAME) return;

	// default to standing on arrival
	if (s->usAnimState != HELIDROP)
	{
		if (fUseAnimation)
		{
			EVENT_InitNewSoldierAnim(s, usAnimState, usAnimCode, TRUE);
		}
		else if (s->ubBodyType != CROW)
		{
			EVENT_InitNewSoldierAnim(s, STANDING, 1, TRUE);
		}
	}

	// ATE: if we are below OK life, make them lie down!
	if (s->bLife < OKLIFE)
	{
		SoldierInSectorIncompaciated(s, s->sInsertionGridNo);
	}
	else if (s->fMercAsleep)
	{
		InternalSoldierInSectorSleep(s, s->sInsertionGridNo);
	}
	else switch (s->bAssignment)
	{
		case PATIENT: SoldierInSectorPatient(s, s->sInsertionGridNo); break;
		case DOCTOR:  SoldierInSectorDoctor( s, s->sInsertionGridNo); break;
		case REPAIR:  SoldierInSectorRepair( s, s->sInsertionGridNo); break;
	}

	// ATE: Make sure movement mode is up to date!
	s->usUIMovementMode = GetMoveStateBasedOnStance(s, gAnimControl[s->usAnimState].ubEndHeight);
}


// IsMercOnTeam() checks to see if the passed in Merc Profile ID is currently on the player's team
BOOLEAN IsMercOnTeam(UINT8 ubMercID)
{
	const SOLDIERTYPE* const s = FindSoldierByProfileIDOnPlayerTeam(ubMercID);
	return s != NULL;
}


BOOLEAN IsMercOnTeamAndInOmertaAlreadyAndAlive(UINT8 ubMercID)
{
	const SOLDIERTYPE* const s = FindSoldierByProfileIDOnPlayerTeam(ubMercID);
	return s != NULL && s->bAssignment != IN_TRANSIT && s->bLife > 0;
}
