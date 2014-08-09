#pragma once

#include "ModLoaderExtensions.h"

class PlayerObject
{
public:
	PlayerObject(ObjectMaster* player);
	PlayerObject();
	
	void Set(ObjectMaster* player);
	static void WritePlayer(ObjectMaster* destination, PlayerObject* source);
	// Teleports source to destination
	static void Teleport(PlayerObject* destination, ObjectMaster* source);

	CharObj1 Data1;
	CharObj2 Data2;

	ObjectMaster* LastPointer;

	// Sonic's Obj2Base
	// Shared with: Shadow, Amy, MetalSonic
	SonicObj2Base Sonic;
	
	// Knuckles's Obj2Base
	// Shared with: Rouge
	KnucklesObj2Base Knuckles;
	
	// Mechless Eggman's Obj2Base
	// Not Shared
	EggmanObj2Base Eggman;
	
	// Mech Eggman's Obj2Base
	// Shared with: Tails Mech
	MechEggmanObj2Base MechEggman;
	
	// Mechless Tails's Obj2Base
	// Not Shared
	TailsObj2Base Tails;

	// Super Sonic's Obj2Base
	// Shared with: Super Shadow
	SuperSonicObj2Base SuperSonic;

private:
	void Initialize();
};
