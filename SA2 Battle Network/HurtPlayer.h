#pragma once

#include <Trampoline.h>

extern Trampoline DamagePlayerHax;
extern Trampoline HurtPlayerHax;
extern Trampoline KillPlayerHax;

extern bool do_damage;

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
