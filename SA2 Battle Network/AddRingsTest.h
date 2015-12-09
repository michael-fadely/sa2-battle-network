#pragma once

#include "Trampoline.h"
#include <SA2ModLoader/SA2Functions.h>

extern Trampoline hax;
extern int DirtyRingHack;

static inline void AddRingsOriginal(char playerNum, int numRings)
{
	void* fptr = hax.Target();
	__asm
	{
		mov edx, [numRings]
		mov al, [playerNum]
		call fptr
	}
}

