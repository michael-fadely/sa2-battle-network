#pragma once

#include <SA2ModLoader.h>

// Sonic's Obj2Base
// Shared with: Shadow, Amy, MetalSonic
struct SonicObj2Base {
	char field_1B8[432];
	short SpindashTimer;
	char filler[42];
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

DataPointer(ObjectMaster*, Player1, &MainCharacter[0]);
DataPointer(ObjectMaster*, Player2, &MainCharacter[1]);

static inline void PlayJingle(const char* song)
{
	PlayJingle(0, song);
}

static inline void PlayMusicOnce(const char* song)
{
	PlayMusicOnce(nullptr, song);
}