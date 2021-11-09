#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "globals.h"
#include "AddHP.h"

using namespace nethax;

static void __stdcall AddHP_cpp(int player_num, float amount)
{
	if (globals::is_connected())
	{
		if (player_num != globals::broker->get_player_number())
		{
			return;
		}

		auto* const data2 = MainCharObj2[player_num];
		const char char_id = data2->CharID;

		if (char_id == Characters_MechTails || char_id == Characters_MechEggman)
		{
			PrintDebug("<< HP SEND: %f, %f", data2->MechHP, amount);
			sws::Packet packet;
			packet << data2->MechHP << amount;
			globals::broker->append(MessageID::P_HP, PacketChannel::reliable, &packet, true);
		}
	}

	events::AddHP_original(player_num, amount);
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

static Trampoline* AddHP_t = nullptr;

void events::AddHP_original(int player_num, float amount)
{
	void* target = AddHP_t->Target();
	__asm
	{
		mov eax, [player_num]
		push amount
		call target
		add esp, 4
	}
}

void events::InitAddHP()
{
	AddHP_t = new Trampoline(reinterpret_cast<intptr_t>(AddHPPtr), static_cast<intptr_t>(0x0046F4C7), AddHP_asm);
}

void events::DeinitAddHP()
{
	delete AddHP_t;
}
