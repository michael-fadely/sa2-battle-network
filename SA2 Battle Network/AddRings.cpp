#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>

#include "Globals.h"
#include "Networking.h"

#include "AddRings.h"

using namespace nethax;

// HACK: Fix for damage ring drop bug
static bool toggle_hack = true;

static void __cdecl AddRings_cpp(int8 playerNum, int32 numRings)
{
	if (toggle_hack && Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		sf::Packet packet;
		packet << RingCount[0] << numRings;
		Globals::Broker->Append(MessageID::S_Rings, Protocol::TCP, &packet, true);
	}

	events::AddRings_original(playerNum, numRings);
}

static void __declspec(naked) AddRings_asm()
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

static Trampoline* AddRingsHax;

void events::AddRings_SyncToggle(bool value)
{
	toggle_hack = value;
}

void events::AddRings_original(char playerNum, int numRings)
{
	void* target = AddRingsHax->Target();
	__asm
	{
		mov edx, [numRings]
		mov al, [playerNum]
		call target
	}
}

static bool MessageHandler(MessageID type, int pnum, sf::Packet& packet)
{
	if (!roundStarted())
		return false;

	int diff;
	packet >> RingCount[1] >> diff;
	PrintDebug(">> RING CHANGE: %d + %d", RingCount[1], diff);
	events::AddRings_original(1, diff);
	return true;
}

void events::InitAddRings()
{
	AddRingsHax = new Trampoline((size_t)AddRingsPtr, (size_t)0x0044CE16, AddRings_asm);
	Globals::Broker->RegisterMessageHandler(MessageID::S_Rings, MessageHandler);
	AddRings_SyncToggle(true);
}

void events::DeinitAddRings()
{
	delete AddRingsHax;
}
