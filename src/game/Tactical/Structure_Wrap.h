#ifndef STRUCURE_WRAP_H
#define STRUCURE_WRAP_H

#include <stdint.h>
#include "Types.h"
#include "Structure.h"
#include "Rotting_Corpses.h"

STRUCTURE *FindCuttableWireFenceAtGridNo(GridNo);

inline bool IsTreePresentAtGridNo(GridNo gn) {
  return FindStructure(gn, STRUCTURE_TREE) != nullptr;
}
inline bool IsFencePresentAtGridNo(GridNo gn) {
  return FindStructure(gn, STRUCTURE_ANYFENCE) != nullptr;
}
inline bool IsRoofPresentAtGridNo(GridNo gn) {
  return FindStructure(gn, STRUCTURE_ROOF) != nullptr;
}
inline bool IsCuttableWireFenceAtGridNo(GridNo gn) {
  return FindCuttableWireFenceAtGridNo(gn) != nullptr;
}
inline bool IsCorpseAtGridNo(GridNo gn, uint8_t level) {
  return GetCorpseAtGridNo(gn, level) != nullptr;
}

BOOLEAN	IsJumpableFencePresentAtGridno( INT16 sGridNo );
BOOLEAN	IsJumpableWindowPresentAtGridNo( INT32 sGridNo, INT8 direction2);

BOOLEAN IsDoorVisibleAtGridNo( INT16 sGridNo );

BOOLEAN	WallExistsOfTopLeftOrientation( INT16 sGridNo );

BOOLEAN	WallExistsOfTopRightOrientation( INT16 sGridNo );

BOOLEAN	WallOrClosedDoorExistsOfTopLeftOrientation( INT16 sGridNo );

BOOLEAN	WallOrClosedDoorExistsOfTopRightOrientation( INT16 sGridNo );

BOOLEAN OpenRightOrientedDoorWithDoorOnRightOfEdgeExists( INT16 sGridNo );
BOOLEAN OpenLeftOrientedDoorWithDoorOnLeftOfEdgeExists( INT16 sGridNo );

STRUCTURE* GetWallStructOfSameOrientationAtGridno(GridNo, INT8 orientation);

BOOLEAN CutWireFence( INT16 sGridNo );

uint8_t IsRepairableStructAtGridNo(GridNo sGridNo, SOLDIERTYPE** tgt);
SOLDIERTYPE* GetRefuelableStructAtGridNo(INT16 sGridNo);

INT16 FindDoorAtGridNoOrAdjacent( INT16 sGridNo );

BOOLEAN SetOpenableStructureToClosed( INT16 sGridNo, UINT8 ubLevel );
#endif
