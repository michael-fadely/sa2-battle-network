#include <SA2ModLoader.h>
#include "Networking.h"		// for MSG
#include "Globals.h"
#include "OnStageChange.h"

void* SetCurrentLevel_ptr = (void*)0x0043D8A0;

void InitOnStageChange()
{
	WriteJump(SetCurrentLevel_ptr, SetCurrentLevel_asm);
}

int __declspec(naked) SetCurrentLevel_asm(int stage)
{
	__asm
	{
		push eax
		call SetCurrentLevel
		pop eax
		retn
	}
}

DataPointer(short, word_1934B84, 0x01934B84);
DataPointer(short, isFirstStageLoad, 0x01748B94);

void SetCurrentLevel(int stage)
{
	using namespace sa2bn::Globals;

	word_1934B84 = CurrentLevel;

	if (isFirstStageLoad)
		isFirstStageLoad = 0;

	if (!isInitialized() || !isConnected())
	{
		CurrentLevel = stage;
		return;
	}

	if (Networking->isServer())
	{
		CurrentLevel = stage;

		Broker->Request(MSG_S_STAGE, true);
		Broker->Finalize();

		// Client sends back MSG_READY
		if (!Broker->WaitForPlayers(Broker->isClientReady))
			return;
	}
	else
	{
		PrintDebug("<> Waiting for stage number...");

		// Server sends MSG_S_STAGE
		if (!Broker->WaitForPlayers(Broker->stageReceived))
			return;

		PrintDebug(">> Received stage change: %d (was %d)", CurrentLevel, stage);

		sf::Packet packet;
		packet << (uint8)MSG_READY;

		Networking->sendSafe(packet);
	}

	PrintDebug(">> Stage received. Resuming game.");
}
