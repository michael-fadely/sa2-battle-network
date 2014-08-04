#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "SA2ModLoader.h"

// Sonic's Obj2Base
// Shared with: Shadow, Amy, MetalSonic
struct SonicObj2Base
{
	char field_1B8[420];

	char HomingAttackTimer;
	char padding_1[7];

	char HomingRangeTimer;
	char padding_2[3];

	unsigned short SpindashTimer;
	char padding_3[46];

	TexList *TextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
};

// Knuckles's Obj2Base
// Shared with: Rouge
struct KnucklesObj2Base
{
	char field_1B8[568];
	TexList *TextureList;
	TexList *EffectTextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
	char field_400[32];
};

// Mechless Eggman's Obj2Base
// Not Shared
struct EggmanObj2Base
{
	char field_1B8[424];
	TexList *TextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
};

// Mech Eggman's Obj2Base
// Shared with: Tails Mech
struct MechEggmanObj2Base
{
	char field_1B8[652];
	TexList *CommonTextureList;
	TexList *TextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
};

// Mechless Tails's Obj2Base
// Not Shared
struct TailsObj2Base
{
	char field_1B8[504];
	TexList *TextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
	char field_3BC[36];
};

// Super Sonic's Obj2Base
// Shared with: Super Shadow
struct SuperSonicObj2Base
{
	char field_1B8[440];
	TexList *TextureList;
	ModelIndex *ModelList;
	AnimationIndex *MotionList;
};

struct InputStruct : ControllerData
{
	unsigned int LastPressed;
	void WriteButtons(ControllerData& destination);

};

DataPointer(ObjectMaster*, Player1, &MainCharacter[0]);
DataPointer(ObjectMaster*, Player2, &MainCharacter[1]);