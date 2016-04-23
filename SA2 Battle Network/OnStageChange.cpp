#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "PacketOverloads.h"
#include "Globals.h"
#include "FunctionPointers.h"
#include "OnStageChange.h"

using namespace nethax;
using namespace Globals;

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
	if (!isConnected())
	{
		SetCurrentLevel_Original(stage);
		return;
	}

	if (Networking->isServer())
	{
		SetCurrentLevel_Original(stage);

		if (Broker->WaitForPlayers(MessageID::S_Stage))
		{
			PrintDebug("<< Sending stage: %d", CurrentLevel);
			PacketEx packet(Protocol::TCP);

			packet.AddType(MessageID::S_Stage);
			packet << CurrentLevel << Current2PLevelGroup;
			Broker->AddTypeSent(MessageID::S_Stage, packet.GetTypeSize(), packet.Protocol);
			packet.Finalize();

			Broker->AddReady(MessageID::S_Stage, packet);
			Broker->Send(packet);
		}
	}
	else
	{
		PrintDebug("<> Waiting for stage number...");
		Broker->SendReady(MessageID::S_Stage);

		if (!Broker->WaitForPlayers(MessageID::S_Stage))
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

	if (!isConnected())
		return;

	if (Networking->isServer())
	{
		// Note that forcing the server to wait for the clients
		// ensures the data arrives at the right time. Otherwise,
		// NextStage could potentially be received before the client
		// reaches this point, thus invalidating the synchronization.
		if (Broker->WaitForPlayers(MessageID::S_NextStage))
		{
			PrintDebug("<< Sending next stage: %d", NextLevel);
			PacketEx packet(Protocol::TCP);

			packet.AddType(MessageID::S_NextStage);
			packet << NextLevel;
			Broker->AddTypeSent(MessageID::S_NextStage, packet.GetTypeSize(), packet.Protocol);
			packet.Finalize();

			Broker->AddReady(MessageID::S_NextStage, packet);
			Broker->Send(packet);
		}
	}
	else
	{
		PrintDebug("<> Waiting for next stage number...");
		Broker->SendReady(MessageID::S_NextStage);

		if (Broker->WaitForPlayers(MessageID::S_NextStage))
			PrintDebug(">> Received next stage: %d", NextLevel);
	}
}

static bool MessageHandler(MessageID type, int pnum, sf::Packet& packet)
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
	Broker->RegisterMessageHandler(MessageID::S_Stage, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::S_NextStage, MessageHandler);
}

void events::DeinitOnStageChange()
{
	delete SetCurrentLevelHax;
	delete SetNextLevelHax;
}
