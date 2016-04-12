#include "stdafx.h"
#include <SA2ModLoader.h>
#include "CharacterSync.h"
#include "Globals.h"
#include "OnStageChange.h"

FunctionPointer(int, Menu_Battle, (void), 0x0066A1A0);
VoidFunc(RandomBattle_SetCharacters, 0x0066B730);

using namespace nethax;
using namespace Globals;

int __cdecl Menu_Battle_hook()
{
	int result = Menu_Battle();

	if (result && isConnected())
	{
		if (Networking->isServer())
		{
			Broker->Request(MessageID::P_Character, true);
			Broker->Finalize();
		}

		Broker->SendReady(MessageID::P_Character);
		Broker->WaitForPlayers(MessageID::P_Character);
	}

	return result;
}

void __cdecl RandomBattle_SetCharacters_hook()
{
	RandomBattle_SetCharacters();
	events::SetCurrentLevel(CurrentLevel);
}

void events::InitCharacterSync()
{
	WriteCall((void*)0x00666325, Menu_Battle_hook);
	WriteCall((void*)0x0066AA76, RandomBattle_SetCharacters_hook);
}
