#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "EmeraldSync.h"

using namespace nethax;

void __cdecl EmeraldLocations_1POr2PGroup3_hook(void* a1);
void __stdcall EmeraldLocations_2PGroup1_hook_c(void* a1);
void __stdcall EmeraldLocations_2PGroup2_hook_c(void* a1);

static uint last_seed = 0;

inline void SetSeed()
{
	last_seed = random::current_seed;
	random::srand_original(FrameCount);
	random::srand_hook(rand());
}

inline void RestoreSeed()
{
	random::srand_original(last_seed);
}

static void __cdecl x_original(void* a1, const Trampoline* t)
{
	SetSeed();

	const void* target = t->Target();
	__asm
	{
		push esi
		mov esi, a1
		call target
		pop esi
	}

	RestoreSeed();
}

static void __declspec(naked) EmeraldLocations_2PGroup1_hook_asm()
{
	__asm
	{
		push esi
		call EmeraldLocations_2PGroup1_hook_c
		ret
	}
}

static void __declspec(naked) EmeraldLocations_2PGroup2_hook_asm()
{
	__asm
	{
		push esi
		call EmeraldLocations_2PGroup2_hook_c
		ret
	}
}

static Trampoline EmeraldLocations_2PGroup1(0x00739340, 0x0073934A, EmeraldLocations_2PGroup1_hook_asm);
static Trampoline EmeraldLocations_2PGroup2(0x007387D0, 0x007387D6, EmeraldLocations_2PGroup2_hook_asm);
static Trampoline EmeraldLocations_1POr2PGroup3(0x007380A0, 0x007380A6, (DetourFunction)EmeraldLocations_1POr2PGroup3_hook);

void __stdcall EmeraldLocations_2PGroup1_hook_c(void* a1)
{
	x_original(a1, &EmeraldLocations_2PGroup1);
}

void __stdcall EmeraldLocations_2PGroup2_hook_c(void* a1)
{
	x_original(a1, &EmeraldLocations_2PGroup2);
}

void __cdecl EmeraldLocations_1POr2PGroup3_hook(void* a1)
{
	SetSeed();

	FunctionPointer(void, EmeraldLocations_1POr2PGroup3_original, (void*), EmeraldLocations_1POr2PGroup3.Target());
	EmeraldLocations_1POr2PGroup3_original(a1);

	RestoreSeed();
}

void events::InitEmeraldSync()
{
	WriteData((Uint8*)0x0073934B, (Uint8)0xEBu);
	WriteData((Uint8*)0x007387E5, (Uint8)0xEBu);
	WriteData((Uint8*)0x007380BD, (Uint8)0xEBu);
}
