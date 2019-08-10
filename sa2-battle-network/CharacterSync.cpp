#include "stdafx.h"
#include <SA2ModLoader.h>
#include "CharacterSync.h"
#include "globals.h"
#include "Networking.h"
#include "OnStageChange.h"

using namespace nethax;
using namespace globals;

static int __cdecl Menu_Battle_hook()
{
	int result = Menu_Battle();

	if (result && is_connected())
	{
		if (!networking->is_server())
		{
			broker->send_ready_and_wait(MessageID::P_Character);
		}
		else if (broker->wait_for_players(MessageID::P_Character))
		{
			PacketEx packet(Protocol::tcp);

			packet.add_type(MessageID::P_Character);

			packet << CurrentCharacter << AltCostume[0] << AltCharacter[0]
				<< CurrentCharacter2P << AltCostume[1] << AltCharacter[1];

			broker->add_type_sent(MessageID::P_Character, packet.get_type_size(), packet.protocol);
			packet.finalize();

			broker->add_ready(MessageID::P_Character, packet);
			broker->send(packet);
		}
	}

	return result;
}

static void __cdecl RandomBattle_SetCharacters_hook()
{
	RandomBattle_SetCharacters();
	events::SetCurrentLevel(CurrentLevel);
}

static bool MessageHandler(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	packet >> CurrentCharacter
		>> AltCostume[0]
		>> AltCharacter[0]
		>> CurrentCharacter2P
		>> AltCostume[1]
		>> AltCharacter[1];

	return true;
}

void events::InitCharacterSync()
{
	WriteCall(reinterpret_cast<void*>(0x00666325), Menu_Battle_hook);
	WriteCall(reinterpret_cast<void*>(0x0066AA76), RandomBattle_SetCharacters_hook);
	broker->register_message_handler(MessageID::P_Character, MessageHandler);
}

void events::DeinitCharacterSync()
{
	WriteCall(reinterpret_cast<void*>(0x00666325), Menu_Battle);
	WriteCall(reinterpret_cast<void*>(0x0066AA76), RandomBattle_SetCharacters);
}
