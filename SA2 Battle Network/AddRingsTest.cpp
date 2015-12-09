#include "stdafx.h"

#include "AddRingsTest.h"

#include "Globals.h"
#include <SA2ModLoader.h>

using namespace nethax;

void hax3(int8 playerNum, int32 numRings);

void __declspec(naked) hax2()
{
	__asm
	{
		push edx
		push eax

		call hax3

		pop eax
		pop edx

		ret
	}
}

Trampoline hax((size_t)AddRingsPtr, (size_t)0x0044CE16, hax2);
int DirtyRingHack = 0;

void hax3(int8 playerNum, int32 numRings)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		// Should this be TCP?
		DirtyRingHack = numRings;
		Globals::Broker->Request(MessageID::S_Rings, true);
	}

	void* target = hax.Target();
	__asm
	{
		mov al, [playerNum]
		mov edx, [numRings]
		call target
	}
}
