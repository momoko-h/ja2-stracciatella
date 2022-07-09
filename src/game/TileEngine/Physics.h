#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "Item_Types.h"
#include "JA2Types.h"
#include "Phys_Math.h"
#include "WorldDef.h"


extern UINT32 guiNumObjectSlots;

enum class TEST_OBJECT : INT8
{
	NONE = 0,
	NO_COLLISIONS,
	NOTWALLROOF_COLLISIONS
};

// Note: to reduce padding, the order of the members differs a little from the
// original order which is still used (and relevant) in LoadSaveRealObject.

struct REAL_OBJECT
{
	BOOLEAN      fAllocated;
	BOOLEAN      fAlive;
	BOOLEAN      fApplyFriction;
	BOOLEAN      fVisible;
	BOOLEAN      fInWater;
	TEST_OBJECT  fTestObject;
	BOOLEAN      fTestEndedWithCollision;
	BOOLEAN      fTestPositionNotSet;

	float        TestZTarget;
	float        AppliedMu;

	vector_3     Position;
	vector_3     TestTargetPosition;
	vector_3     OldPosition;
	vector_3     Velocity;
	vector_3     OldVelocity;
	vector_3     InitialForce;
	vector_3     Force;
	vector_3     CollisionNormal;
	vector_3     CollisionVelocity;
	float        CollisionElasticity;

	LEVELNODE    *pNode;
	LEVELNODE    *pShadow;

	INT16        sConsecutiveCollisions;
	INT16        sConsecutiveZeroVelocityCollisions;
	INT32        iOldCollisionCode;

	FLOAT        dLifeLength;
	FLOAT        dLifeSpan;
	OBJECTTYPE   Obj;
	SOLDIERTYPE* owner;
	SOLDIERTYPE* target;
	GridNo       sGridNo;
	GridNo       sFirstGridNo;
	BOOLEAN      fFirstTimeMoved;
	UINT8        ubActionCode;
	BOOLEAN      fDropItem;
	UINT32       uiNumTilesMoved;
	BOOLEAN      fCatchGood;
	BOOLEAN      fAttemptedCatch;
	BOOLEAN      fCatchAnimOn;
	BOOLEAN      fCatchCheckDone;
	BOOLEAN      fEndedWithCollisionPositionSet;
	vector_3     EndedWithCollisionPosition;
	BOOLEAN      fHaveHitGround;
	UINT8        ubLastTargetTakenDamage;
	GridNo       sLevelNodeGridNo;
	UINT32       uiSoundID;
};


// OBJECT LIST STUFF
REAL_OBJECT* CreatePhysicalObject(const OBJECTTYPE* pGameObj, float dLifeLength, float xPos, float yPos, float zPos, float xForce, float yForce, float zForce, SOLDIERTYPE* owner, UINT8 ubActionCode, SOLDIERTYPE* target);
void RemoveAllPhysicsObjects(void);


BOOLEAN CalculateLaunchItemChanceToGetThrough(const SOLDIERTYPE* pSoldier, const OBJECTTYPE* pItem, INT16 sGridNo, UINT8 ubLevel, INT16 sEndZ, INT16* psFinalGridNo, BOOLEAN fArmed, INT8* pbLevel, BOOLEAN fFromUI);

void CalculateLaunchItemParamsForThrow(SOLDIERTYPE* pSoldier, INT16 sGridNo, UINT8 ubLevel, INT16 sZPos, OBJECTTYPE* pItem, INT8 bMissBy, UINT8 ubActionCode, SOLDIERTYPE* target);


// SIMULATE WORLD
void SimulateWorld(void);


void SavePhysicsTableToSaveGameFile(HWFILE);
void LoadPhysicsTableFromSavedGameFile(HWFILE);

#endif
