#include "stdafx.h"

#include "AddRings.h"

#include "Globals.h"
#include <SA2ModLoader.h>

using namespace nethax;

void AddRings_cpp(int8 playerNum, int32 numRings);

void __declspec(naked) AddRings_asm()
{
	__asm
	{
		push edx
		push eax

		call AddRings_cpp

		pop eax
		pop edx

		ret
	}
}

Trampoline events::AddRingsHax((size_t)AddRingsPtr, (size_t)0x0044CE16, AddRings_asm);
int events::DirtyRingHack = 0;

void AddRings_cpp(int8 playerNum, int32 numRings)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		// Should this be TCP?
		events::DirtyRingHack = numRings;
		Globals::Broker->Request(MessageID::S_Rings, Protocol::TCP);
	}

	void* target = events::AddRingsHax.Target();
	__asm
	{
		mov al, [playerNum]
		mov edx, [numRings]
		call target
	}
}
