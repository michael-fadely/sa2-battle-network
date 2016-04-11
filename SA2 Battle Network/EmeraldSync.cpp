#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "FrameSync.h"

void __cdecl sub_7380A0_hook(void* a1);
void __stdcall sub_739340_hook_c(void* a1);
void __stdcall sub_7387D0_hook_c(void* a1);

static void __cdecl x_original(void* a1, const Trampoline* t)
{
	const void* target = t->Target();
	__asm
	{
		push esi
		mov esi, a1
		call target
		pop esi
	}
}

static void __declspec(naked) sub_739340_hook_asm()
{
	__asm
	{
		push esi
		call sub_739340_hook_c
		ret
	}
}

static void __declspec(naked) sub_7387D0_hook_asm()
{
	__asm
	{
		push esi
		call sub_7387D0_hook_c
		ret
	}
}

static Trampoline sub_739340(0x00739340, 0x0073934A, sub_739340_hook_asm);
static Trampoline sub_7387D0(0x007387D0, 0x007387D6, sub_7387D0_hook_asm);
static Trampoline sub_7380A0(0x007380A0, 0x007380A6, (DetourFunction)sub_7380A0_hook);

void __stdcall sub_739340_hook_c(void* a1)
{
	FrameSync();
	x_original(a1, &sub_739340);
}

void __stdcall sub_7387D0_hook_c(void* a1)
{
	FrameSync();
	x_original(a1, &sub_7387D0);
}

void __cdecl sub_7380A0_hook(void* a1)
{
	FrameSync();
	FunctionPointer(void, sub_7380A0_original, (void*), sub_7380A0.Target());
	sub_7380A0_original(a1);
}
