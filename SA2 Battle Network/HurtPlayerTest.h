#pragma once

#include "Trampoline.h"

extern Trampoline HurtPlayerHax;
extern Trampoline KillPlayerHax;

static inline void KillPlayerOriginal(int playerNum)
{
	void* target = KillPlayerHax.Target();
	__asm
	{
		push ebx
		mov ebx, playerNum
		call target
		pop ebx
	}
}
