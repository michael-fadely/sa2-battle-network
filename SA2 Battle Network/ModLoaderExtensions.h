#pragma once

#include <Windows.h>

#include "SA2ModLoader.h"

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

struct InputStruct : ControllerData
{
	InputStruct() : ControllerData()
	{
		LastPressed = 0;
	}
	unsigned int LastPressed;
	void WriteButtons(ControllerData& destination);

};

DataPointer(ObjectMaster*, Player1, &MainCharacter[0]);
DataPointer(ObjectMaster*, Player2, &MainCharacter[1]);

// Stops music playback
VoidFunc(StopMusic, 0x00442F50);
// Resets music playback to the last song as specified
// by PlayMusic
VoidFunc(ResetMusic, 0x00442D90);

static const void *const PlayJinglePtr = (void*)0x00443480;
// Plays specified song once, then restores previous song
// as set by PlayMusic.
static inline void PlayJingle(int a1, const char *song)
{
	__asm
	{
		mov ecx, [a1]
		mov ebx, [song]
		call PlayJinglePtr
	}
}

static const void *const PlayOncePtr = (void*)0x00442EF0;
// Plays the specified song once.
// Takes effect immediately.
static inline void PlayOnce(void *a1, const char *a2)
{
	__asm
	{
		mov ecx, [a1]
		mov edi, [a2]
		call PlayOncePtr
	}
}

static const void *const _PlayOncePtr = (void*)0x00442E60;
// Plays the specified song once.
// Requires StopMusic and ResetMusic to be called.
static inline void _PlayOnce(const char *a1)
{
	__asm
	{
		mov edi, [a1]
		call _PlayOncePtr
	}
}