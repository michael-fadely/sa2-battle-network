#include "stdafx.h"
#include <SA2ModLoader.h>
#include "Globals.h"
#include "AddHP.h"

using namespace nethax;

void __stdcall AddHP_cpp(int playerNum, float amount);
float events::DirtyHPHack = 0.0f;

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

Trampoline events::AddHPHax((size_t)AddHPPtr, (size_t)0x0046F4C7, AddHP_asm);

void __stdcall AddHP_cpp(int playerNum, float amount)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		char charid = MainCharObj2[playerNum]->CharID;
		if (charid == Characters_MechTails || charid == Characters_MechEggman)
		{
			// Should this be TCP?
			events::DirtyHPHack = amount;
			Globals::Broker->Request(MessageID::P_HP, true);
		}
	}

	events::AddHPOriginal(playerNum, amount);
}
