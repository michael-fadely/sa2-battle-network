#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "Globals.h"
#include "OnStageChange.h"

using namespace nethax;
using namespace Globals;

//DataPointer(short, isFirstStageLoad, 0x01748B94);

void __cdecl SetNextLevel_Hook();

Trampoline SetCurrentLevelHax(0x0043D8A0, 0x0043D8A7, events::SetCurrentLevel_asm);
Trampoline SetNextLevelHax(0x0043C4D0, 0x0043C4D5, SetNextLevel_Hook);

void __declspec(naked) events::SetCurrentLevel_asm()
{
	__asm
	{
		push eax
		call SetCurrentLevel
		ret
	}
}

inline void SetCurrentLevel_Original(short stage)
{
	void* target = SetCurrentLevelHax.Target();
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
			PacketEx packet(Protocol::TCP);
			Broker->Request(MessageID::S_Stage, packet);
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

void __cdecl SetNextLevel_Hook()
{
	// Immediately calling in case it does any other magic I'm not aware of.
	VoidFunc(original, SetNextLevelHax.Target());
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
			PacketEx packet(Protocol::TCP);
			Broker->Request(MessageID::S_NextStage, packet);
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
