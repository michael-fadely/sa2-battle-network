#include "stdafx.h"

#include <Windows.h>
#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Random.h"
#include "AddressList.h"
#include "typedefs.h"
#include "EmeraldSync.h"
#include "FunctionPointers.h"

using namespace nethax;

static void __cdecl EmeraldLocations_1POr2PGroup3_hook(void* a1);
static void __stdcall EmeraldLocations_2PGroup1_hook_c(void* a1);
static void __stdcall EmeraldLocations_2PGroup2_hook_c(void* a1);

static uint last_seed = 0;

inline void SetSeed()
{
	last_seed = random::current_seed;
	random::srand_hook(GetTickCount() + FrameCount);

#ifdef _DEBUG
	PrintDebug("Seed stored/generated: 0x%08X/0x%08X", last_seed, random::current_seed);
#endif
}

inline void RestoreSeed()
{
	random::current_seed = last_seed;
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

static Trampoline* EmeraldLocations_2PGroup1_t;
static Trampoline* EmeraldLocations_2PGroup2_t;
static Trampoline* EmeraldLocations_1POr2PGroup3_t;

static void __stdcall EmeraldLocations_2PGroup1_hook_c(void* a1)
{
	x_original(a1, EmeraldLocations_2PGroup1_t);
}

static void __stdcall EmeraldLocations_2PGroup2_hook_c(void* a1)
{
	x_original(a1, EmeraldLocations_2PGroup2_t);
}

static void __cdecl EmeraldLocations_1POr2PGroup3_hook(void* a1)
{
	SetSeed();

	_FunctionPointer(void, EmeraldLocations_1POr2PGroup3_original, (void*), EmeraldLocations_1POr2PGroup3_t->Target());
	EmeraldLocations_1POr2PGroup3_original(a1);

	RestoreSeed();
}

void events::InitEmeraldSync()
{
	WriteData((Uint8*)0x0073934B, (Uint8)0xEBu);
	WriteData((Uint8*)0x007387E5, (Uint8)0xEBu);
	WriteData((Uint8*)0x007380BD, (Uint8)0xEBu);

	EmeraldLocations_2PGroup1_t = new Trampoline(0x00739340, 0x0073934A, EmeraldLocations_2PGroup1_hook_asm);
	EmeraldLocations_2PGroup2_t = new Trampoline(0x007387D0, 0x007387D6, EmeraldLocations_2PGroup2_hook_asm);
	EmeraldLocations_1POr2PGroup3_t = new Trampoline(0x007380A0, 0x007380A6, EmeraldLocations_1POr2PGroup3_hook);
}

void events::DeinitEmeraldSync()
{
	WriteData((Uint8*)0x0073934B, (Uint8)0x75u);
	WriteData((Uint8*)0x007387E5, (Uint8)0x75u);
	WriteData((Uint8*)0x007380BD, (Uint8)0x75u);

	delete EmeraldLocations_2PGroup1_t;
	delete EmeraldLocations_2PGroup2_t;
	delete EmeraldLocations_1POr2PGroup3_t;
}
