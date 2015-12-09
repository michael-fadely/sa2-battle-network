#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Networking.h"
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
	using namespace nethax;
	using namespace Globals;

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

		Broker->Request(MessageID::S_Stage, true);
		Broker->Finalize();
		Broker->SendReady(MessageID::S_Stage);

		if (!Broker->WaitForPlayers(MessageID::S_Stage))
			return;
	}
	else
	{
		PrintDebug("<> Waiting for stage number...");

		if (!Broker->WaitForPlayers(MessageID::S_Stage))
		{
			CurrentLevel = stage;
			return;
		}

		PrintDebug(">> Received stage change: %d (was %d)", CurrentLevel, stage);

		Broker->SendReady(MessageID::S_Stage);
	}

	PrintDebug(">> Stage received. Resuming game.");
}
