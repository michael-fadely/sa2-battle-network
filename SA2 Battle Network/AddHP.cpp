#include "stdafx.h"

#include "AddHP.h"

#include "Globals.h"
#include <SA2ModLoader.h>

using namespace nethax;

void __stdcall AddHP_cpp(int playerNum, float amount);
float DirtyHPHack = 0.0f;

void __declspec(naked) AddHP_asm()
{
	__asm
	{
		push [esp+4]
		push eax
		call AddHP_cpp
		retn
	}
}

Trampoline AddHPHax((size_t)AddHPPtr, (size_t)0x0046F4C7, AddHP_asm);

void __stdcall AddHP_cpp(int playerNum, float amount)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		// Should this be TCP?
		DirtyHPHack = amount;
		Globals::Broker->Request(MessageID::P_HP, true);
	}

	void* target = AddHPHax.Target();
	__asm
	{
		mov eax, [playerNum]
		push amount
		call target
		add esp, 4
	}
}
