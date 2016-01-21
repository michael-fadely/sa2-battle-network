#pragma once

#include "ModLoaderExtensions.h"

class PlayerObject
{
public:
	explicit PlayerObject(ObjectMaster* player);
	PlayerObject();

	/// <summary>
	/// Copies CharObj1 and CharObj2 data from an ObjectMaster to this instance.
	/// </summary>
	/// <param name="source">The ObjectMaster containing the information to copy to this instance.</param>
	void Copy(ObjectMaster* source);
	/// <summary>
	/// Applies a PlayerObject to an ObjectMaster.
	/// </summary>
	/// <param name="destination">The destination ObjectMaster to apply changes to.</param>
	/// <param name="source">The changes to apply.</param>
	static void WritePlayer(ObjectMaster* destination, PlayerObject* source);

	CharObj1 Data1;
	CharObj2 Data2;

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
	ObjectMaster* lastObject;
};
