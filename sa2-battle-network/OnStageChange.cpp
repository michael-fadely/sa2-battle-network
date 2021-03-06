#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "PacketOverloads.h"
#include "globals.h"
#include "FunctionPointers.h"
#include "OnStageChange.h"

using namespace nethax;
using namespace globals;

//DataPointer(short, isFirstStageLoad, 0x01748B94);

static void __declspec(naked) SetCurrentLevel_asm()
{
	__asm
	{
		push eax
		call events::SetCurrentLevel
		ret
	}
}

static void __cdecl SetNextLevel_Hook();

Trampoline* SetCurrentLevelHax;
Trampoline* SetNextLevelHax;

inline void SetCurrentLevel_Original(short stage)
{
	void* target = SetCurrentLevelHax->Target();
	__asm
	{
		movzx eax, stage
		call target
	}
}

void __stdcall events::SetCurrentLevel(short stage)
{
	if (!is_connected())
	{
		SetCurrentLevel_Original(stage);
		return;
	}

	if (networking->is_server())
	{
		SetCurrentLevel_Original(stage);

		if (broker->wait_for_players(MessageID::S_Stage))
		{
			PrintDebug("<< Sending stage: %d", CurrentLevel);
			PacketEx packet(Protocol::tcp);

			packet.add_type(MessageID::S_Stage);
			packet << CurrentLevel << Current2PLevelGroup;
			broker->add_type_sent(MessageID::S_Stage, packet.get_type_size(), packet.protocol);
			packet.finalize();

			broker->add_ready(MessageID::S_Stage, packet);
			broker->send(packet);
		}
	}
	else
	{
		PrintDebug("<> Waiting for stage number...");
		if (!broker->send_ready_and_wait(MessageID::S_Stage))
		{
			SetCurrentLevel_Original(stage);
			return;
		}

		PrintDebug(">> Received stage change: %d (was %d)", CurrentLevel, stage);
	}

	PrintDebug(">> Stage received. Resuming game.");
}

static void __cdecl SetNextLevel_Hook()
{
	// Immediately calling in case it does any other magic I'm not aware of.
	_VoidFunc(original, SetNextLevelHax->Target());
	original();

	if (!is_connected())
		return;

	if (networking->is_server())
	{
		// Note that forcing the server to wait for the clients
		// ensures the data arrives at the right time. Otherwise,
		// NextStage could potentially be received before the client
		// reaches this point, thus invalidating the synchronization.
		if (broker->wait_for_players(MessageID::S_NextStage))
		{
			PrintDebug("<< Sending next stage: %d", NextLevel);
			PacketEx packet(Protocol::tcp);

			packet.add_type(MessageID::S_NextStage);
			packet << NextLevel;
			broker->add_type_sent(MessageID::S_NextStage, packet.get_type_size(), packet.protocol);
			packet.finalize();

			broker->add_ready(MessageID::S_NextStage, packet);
			broker->send(packet);
		}
	}
	else
	{
		PrintDebug("<> Waiting for next stage number...");

		if (broker->send_ready_and_wait(MessageID::S_NextStage))
		{
			PrintDebug(">> Received next stage: %d", NextLevel);
		}
	}
}

static bool MessageHandler(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	switch (type)
	{
		default:
			return false;

		case MessageID::S_Stage:
			packet >> CurrentLevel >> Current2PLevelGroup;
			break;

		case MessageID::S_NextStage:
			packet >> NextLevel;
			break;
	}

	return true;
}

void events::InitOnStageChange()
{
	SetCurrentLevelHax = new Trampoline(0x0043D8A0, 0x0043D8A7, SetCurrentLevel_asm);
	SetNextLevelHax = new Trampoline(0x0043C4D0, 0x0043C4D5, SetNextLevel_Hook);
	broker->register_message_handler(MessageID::S_Stage, MessageHandler);
	broker->register_message_handler(MessageID::S_NextStage, MessageHandler);
}

void events::DeinitOnStageChange()
{
	delete SetCurrentLevelHax;
	delete SetNextLevelHax;
}
