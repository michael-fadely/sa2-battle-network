#pragma once

#include "ModLoaderExtensions.h"

class PlayerObject
{
public:
	explicit PlayerObject(ObjectMaster* player);
	PlayerObject();

	/**
	 * \brief Copies CharObj1 and CharObj2 data from an ObjectMaster to this instance.
	 * \param source The ObjectMaster containing the information to copy to this instance.
	 */
	void copy(const ObjectMaster* source);

	/**
	 * \brief Applies a PlayerObject to an ObjectMaster.
	 * \param destination The destination ObjectMaster to apply changes to.
	 * \param source The changes to apply.
	 */
	static void write_player(ObjectMaster* destination, const PlayerObject* source);

	CharObj1 data1 {};
	CharObj2 data2 {};

	// Sonic's Obj2Base
	// Shared with: Shadow, Amy, MetalSonic
	SonicObj2Base sonic {};

	// Knuckles's Obj2Base
	// Shared with: Rouge
	KnucklesObj2Base knuckles {};

	// Mechless Eggman's Obj2Base
	// Not Shared
	EggmanObj2Base eggman {};

	// Mech Eggman's Obj2Base
	// Shared with: Tails Mech
	MechEggmanObj2Base mech_eggman {};

	// Mechless Tails's Obj2Base
	// Not Shared
	TailsObj2Base tails {};

	// Super Sonic's Obj2Base
	// Shared with: Super Shadow
	SuperSonicObj2Base super_sonic {};

private:
	void initialize();
	const ObjectMaster* last_object = nullptr;
};
