#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Trampoline.h"
#include "Networking.h"
#include "Globals.h"
#include "OnStageChange.h"

//DataPointer(short, isFirstStageLoad, 0x01748B94);

Trampoline SetCurrentLevelHax(0x0043D8A0, 0x0043D8A7, SetCurrentLevel_asm);

void __declspec(naked) SetCurrentLevel_asm()
{
	__asm
	{
		push eax
		call SetCurrentLevel
		pop eax
		ret
	}
}

inline void SetCurrentLevel_Original(short stage)
{
	void* original = SetCurrentLevelHax.Target();
	__asm
	{
		mov ax, stage
		call original
	}
}

void __cdecl SetCurrentLevel(short stage)
{
	using namespace nethax;
	using namespace Globals;

	if (!isConnected())
	{
		SetCurrentLevel_Original(stage);
		return;
	}

	if (Networking->isServer())
	{
		SetCurrentLevel_Original(stage);

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
			SetCurrentLevel_Original(stage);
			return;
		}

		PrintDebug(">> Received stage change: %d (was %d)", CurrentLevel, stage);

		Broker->SendReady(MessageID::S_Stage);
	}

	PrintDebug(">> Stage received. Resuming game.");
}
