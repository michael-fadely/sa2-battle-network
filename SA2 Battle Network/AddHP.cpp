#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "Globals.h"
#include "AddHP.h"

using namespace nethax;

static void __stdcall AddHP_cpp(int playerNum, float amount)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		auto data2 = MainCharObj2[playerNum];
		char charid = data2->CharID;
		if (charid == Characters_MechTails || charid == Characters_MechEggman)
		{
			PrintDebug("<< HP SEND: %f, %f", data2->MechHP, amount);
			sf::Packet packet;
			packet << data2->MechHP << amount;
			Globals::Broker->Append(MessageID::P_HP, Protocol::TCP, &packet, true);
		}
	}

	events::AddHP_original(playerNum, amount);
}

static void __declspec(naked) AddHP_asm()
{
	__asm
	{
		push [esp+4]
		push eax
		call AddHP_cpp
		retn
	}
}

static Trampoline* AddHPHax;

void events::AddHP_original(int playerNum, float amount)

{
	void* target = AddHPHax->Target();
	__asm
	{
		mov eax, [playerNum]
		push amount
		call target
		add esp, 4
	}
}

void events::InitAddHP()
{
	AddHPHax = new Trampoline((size_t)AddHPPtr, (size_t)0x0046F4C7, AddHP_asm);
}

void events::DeinitAddHP()
{
	delete AddHPHax;
}
