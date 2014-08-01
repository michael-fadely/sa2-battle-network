#pragma once

#include "SA2ModLoader.h"

class PlayerObject : ObjectMaster
{
public:
	inline SonicCharObj2*		SonicData()			{ return (SonicCharObj2*)Data2; }
	inline KnucklesCharObj2*	KnucklesData()		{ return (KnucklesCharObj2*)Data2; }
	inline EggmanCharObj2*		EggmanData()		{ return (EggmanCharObj2*)Data2; }
	inline MechEggmanCharObj2*	EggmanMechData()	{ return (MechEggmanCharObj2*)Data2; }
	inline TailsCharObj2*		TailsData()			{ return (TailsCharObj2*)Data2; }
	inline SuperSonicCharObj2*	SuperSonicData()	{ return (SuperSonicCharObj2*)Data2; }
};