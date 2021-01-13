#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>

#include "globals.h"
#include "Networking.h"

#include "AddRings.h"

using namespace nethax;

// HACK: Fix for damage ring drop bug
// TODO: implement arbitrary data read & consumption from packet in PacketEx
static bool toggle_hack = true;

static void __cdecl AddRings_cpp(int8_t player_num, int32_t num_rings)
{
	if (toggle_hack && globals::is_connected())
	{
		if (player_num != globals::broker->get_player_number())
		{
			return;
		}

		sws::Packet packet;
		packet << RingCount[player_num] << num_rings;
		globals::broker->append(MessageID::S_Rings, Protocol::tcp, &packet, true);
	}

	events::AddRings_original(player_num, num_rings);
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

static Trampoline* AddRings_trampoline;

void events::AddRings_sync_toggle(bool value)
{
	toggle_hack = value;
}

void events::AddRings_original(int8_t player_num, int32_t num_rings)
{
	void* target = AddRings_trampoline->Target();
	__asm
	{
		mov edx, [num_rings]
		mov al, [player_num]
		call target
	}
}

static bool message_handler(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	if (!round_started())
	{
		return false;
	}

	int diff;
	packet >> RingCount[pnum] >> diff;
	PrintDebug(">> RING CHANGE: %d + %d", RingCount[pnum], diff);
	events::AddRings_original(pnum, diff);

	return true;
}

void events::InitAddRings()
{
	AddRings_trampoline = new Trampoline(reinterpret_cast<intptr_t>(AddRingsPtr), static_cast<intptr_t>(0x0044CE16), &AddRings_asm);
	globals::broker->register_message_handler(MessageID::S_Rings, message_handler);
	AddRings_sync_toggle(true);
}

void events::DeinitAddRings()
{
	delete AddRings_trampoline;
}
