#pragma once

#include <Trampoline.h>

extern Trampoline AddRingsHax;
extern int DirtyRingHack;

static inline void AddRingsOriginal(char playerNum, int numRings)
{
	void* fptr = AddRingsHax.Target();
	__asm
	{
		mov edx, [numRings]
		mov al, [playerNum]
		call fptr
	}
}

