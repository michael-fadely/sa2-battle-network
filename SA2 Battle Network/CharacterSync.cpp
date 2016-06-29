#include "stdafx.h"
#include <SA2ModLoader.h>
#include "CharacterSync.h"
#include "Globals.h"
#include "Networking.h"
#include "PacketOverloads.h"
#include "OnStageChange.h"

FunctionPointer(int, Menu_Battle, (void), 0x0066A1A0);
VoidFunc(RandomBattle_SetCharacters, 0x0066B730);

using namespace nethax;
using namespace Globals;

static int __cdecl Menu_Battle_hook()
{
	int result = Menu_Battle();

	if (result && isConnected())
	{
		if (Networking->isServer())
		{
			if (Broker->WaitForPlayers(MessageID::P_Character))
			{
				PacketEx packet(Protocol::TCP);

				packet.AddType(MessageID::P_Character);
				packet << CurrentCharacter << AltCostume[0] << AltCharacter[0]
					<< CurrentCharacter2P << AltCostume[1] << AltCharacter[1];
				Broker->AddTypeSent(MessageID::P_Character, packet.GetTypeSize(), packet.Protocol);
				packet.Finalize();

				Broker->AddReady(MessageID::P_Character, packet);
				Broker->Send(packet);
			}
		}
		else
		{
			Broker->SendReadyAndWait(MessageID::P_Character);
		}
	}

	return result;
}

static void __cdecl RandomBattle_SetCharacters_hook()
{
	RandomBattle_SetCharacters();
	events::SetCurrentLevel(CurrentLevel);
}

static bool MessageHandler(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	packet >> CurrentCharacter >> AltCostume[0] >> AltCharacter[0]
		>> CurrentCharacter2P >> AltCostume[1] >> AltCharacter[1];
	return true;
}

void events::InitCharacterSync()
{
	WriteCall((void*)0x00666325, Menu_Battle_hook);
	WriteCall((void*)0x0066AA76, RandomBattle_SetCharacters_hook);
	Broker->RegisterMessageHandler(MessageID::P_Character, MessageHandler);
}

void events::DeinitCharacterSync()
{
	WriteCall((void*)0x00666325, Menu_Battle);
	WriteCall((void*)0x0066AA76, RandomBattle_SetCharacters);
}
