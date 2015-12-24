#pragma once

#include "Trampoline.h"

extern Trampoline AddHPHax;
extern float DirtyHPHack;

static inline void AddHPOriginal(int playerNum, float amount)
{
	void* AddHPPtr = AddHPHax.Target();
	__asm
	{
		mov eax, [playerNum]
		push amount
		call AddHPPtr
		add esp, 4
	}
}
