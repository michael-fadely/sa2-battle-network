#pragma once

#include <Trampoline.h>

extern Trampoline AddHPHax;
extern float DirtyHPHack;

static inline void AddHPOriginal(int playerNum, float amount)
{
	void* target = AddHPHax.Target();
	__asm
	{
		mov eax, [playerNum]
		push amount
		call target
		add esp, 4
	}
}
