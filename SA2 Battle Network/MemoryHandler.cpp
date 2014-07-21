#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LazyMemory.h"

#include "Common.h"
#include "CommonEnums.h"
#include "AddressList.h"
#include "ActionBlacklist.h"

#include "MemoryManagement.h"
#include "PlayerObject.h"
#include "InputStruct.h"
#include "MemoryStruct.h"

#include "Networking.h"
#include "QuickSock.h"
#include "PacketHandler.h"

#include "MemoryHandler.h"

using namespace std;

/*
//	Memory Handler Class
*/

MemoryHandler::MemoryHandler(PacketHandler* packetHandler, bool isserver)
{
	this->packetHandler = packetHandler;
	this->isServer = isserver;
	
	firstMenuEntry = false;
	wroteP2Start = false;
	splitToggled = false;
	Teleported = false;
	writePlayer = false;

	InitPlayers();
	InitInput();
	
	cout << "[MemoryHandler] Initializing Memory Structures" << endl;

	memset(&local, 0x00, sizeof(MemStruct));
	memset(&remote, 0x00, sizeof(MemStruct));

	memset(&recvPlayer, 0x00, sizeof(AbstractPlayer));
	memset(&recvInput, 0x00, sizeof(abstractInput));

	memset(&sendPlayer, 0x00, sizeof(AbstractPlayer));
	memset(&sendInput, 0x00, sizeof(abstractInput));

	thisFrame = 0;
	lastFrame = 0;

	return;
}
MemoryHandler::~MemoryHandler()
{
	packetHandler = nullptr;
	cout << "<> [MemoryHandler::~MemoryHandler] Deinitializing Player Objects and Input Structures" << endl;
	DeinitPlayers();
	DeinitInput();
}

void MemoryHandler::InitPlayers()
{
	if (player1 == nullptr)
		player1 = new PlayerObject(ADDR_PLAYER1);
	else
		cout << "<> [MemoryHandler::InitPlayers] Player 1 has already been initialized." << endl;

	if (player2 == nullptr)
		player2 = new PlayerObject(ADDR_PLAYER2);
	else
		cout << "<> [MemoryHandler::InitPlayers] Player 2 has already been initialized." << endl;

	return;
}
void MemoryHandler::DeinitPlayers()
{
	if (player1 != nullptr)
	{
		delete player1;
		player1 = nullptr;
	}
	if (player2 != nullptr)
	{
		delete player2;
		player2 = nullptr;
	}

	return;
}

void MemoryHandler::InitInput()
{
	if (p1Input == nullptr)
		p1Input = new InputStruct(ADDR_P1INPUT);
	else
		cout << "<> [MemoryHandler::InitInput] P1 Input has already been initialized." << endl;

	if (p2Input == nullptr)
		p2Input = new InputStruct(ADDR_P2INPUT);
	else
		cout << "<> [MemoryHandler::InitInput] P2 Input has already been initialized." << endl;

	return;
}
void MemoryHandler::DeinitInput()
{
	if (p1Input != nullptr)
	{
		delete p1Input;
		p1Input = nullptr;
	}
	if (p2Input != nullptr)
	{
		delete p2Input;
		p2Input = nullptr;
	}

	return;
}

void MemoryHandler::SendSystem(QSocket* Socket)
{
	/*
	if (local.system.GameState > GameState::INGAME && remote.system.GameState == GameState::INACTIVE)
	{
		if (remote.game.stage != 0)
		{
			cout << "<> Fixing stage number..." << endl;
			local.game.stage = 0;
			WriteMemory(ADDR_STAGE, &local.game.stage, sizeof(char));
		}
		local.system.GameState = remote.system.GameState;
	}
	*/

	if (remote.system.GameState >= GameState::LOAD_FINISHED && remote.system.multiMode > 0)
	{
		if (local.system.GameState != remote.system.GameState && remote.system.GameState > GameState::LOAD_FINISHED)
		{
			cout << "<< Sending gamestate [" << (ushort)remote.system.GameState << "]" << endl;

			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_GAMESTATE);
			Socket->writeByte(remote.system.GameState);

			packetHandler->SendMsg(true);
			local.system.GameState = remote.system.GameState;
		}

		if (local.system.PauseMenuSelection != remote.system.PauseMenuSelection)
		{
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_PAUSESEL);
			Socket->writeByte(remote.system.PauseMenuSelection);

			packetHandler->SendMsg(true);
			local.system.PauseMenuSelection = remote.system.PauseMenuSelection;
		}

		if (local.game.Time[1] != remote.game.Time[1] && isServer)
		{
			//cout << "<< Sending time [" << (ushort)local.game.Time[0] << ":" << (ushort)local.game.Time[1] << "]" << endl;
			Socket->writeByte(MSG_NULL); Socket->writeByte(2);
			Socket->writeByte(MSG_S_TIME);
			Socket->writeByte(MSG_KEEPALIVE);
			Socket->writeBytes(&remote.game.Time[0], sizeof(char)* 3);

			packetHandler->SendMsg();
			memcpy(&local.game.Time[0], &remote.game.Time[0], sizeof(char)* 3);
			packetHandler->setSendKeepalive();
		}

		if (local.game.TimeStop != remote.game.TimeStop)
		{
			cout << "<< Sending time stop [";
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_TIMESTOP);
			
			// Swap the value since player 1 is relative to the client

			if (remote.game.TimeStop == 1)
			{
				cout << 2 << "]" << endl;
				Socket->writeByte(2);
			}
			else if (remote.game.TimeStop == 2)
			{
				cout << 1 << "]" << endl;
				Socket->writeByte(1);
			}
			else if (remote.game.TimeStop == 0)
			{
				cout << 0 << "]" << endl;
				Socket->writeByte(0);
			}
			
			packetHandler->SendMsg(true);

			local.game.TimeStop = remote.game.TimeStop;
		}
	}
	if (remote.system.GameState != GameState::INGAME || remote.game.TimeStop > 0 || !isServer)
	{
		if (Duration(packetHandler->getSendKeepalive()) >= 1000)
		{
			Socket->writeByte(MSG_NULL); Socket->writeByte(1);
			Socket->writeByte(MSG_KEEPALIVE);

			packetHandler->SendMsg();
			packetHandler->setSendKeepalive();
		}
	}
}
void MemoryHandler::SendInput(QSocket* Socket, uint sendTimer)
{
	if (remote.menu.main == 16 || remote.system.multiMode > 0 && remote.system.GameState > GameState::INACTIVE)
	{
		p1Input->read();

		if (!CheckFrame())
			ToggleSplitscreen();

		if (sendInput.buttons.Held != p1Input->buttons.Held && remote.system.GameState != GameState::MEMCARD)
		{
			//cout << "<< Sending buttons!" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
				
			Socket->writeByte(MSG_I_BUTTONS);
			Socket->writeInt(p1Input->buttons.Held);

			packetHandler->SendMsg(true);

			sendInput.buttons.Held = p1Input->buttons.Held;

		}
	
		if (sendInput.analog.x != p1Input->analog.x || sendInput.analog.y != p1Input->analog.y)
		{
			if (remote.system.GameState == GameState::INGAME)
			{
				if (Duration(analogTimer) >= 125)
				{
					if (p1Input->analog.x == 0 && p1Input->analog.y == 0)
					{
						//cout << "<< Analog 0'd out; sending reliably." << endl;
						packetHandler->WriteReliable(); Socket->writeByte(1);
						Socket->writeByte(MSG_I_ANALOG);
						Socket->writeShort(p1Input->analog.x);
						Socket->writeShort(p1Input->analog.y);

						packetHandler->SendMsg(true);

						sendInput.analog.x = p1Input->analog.x;
						sendInput.analog.y = p1Input->analog.y;
					}
					else
					{
						//cout << "<< Sending analog!" << endl;
						Socket->writeByte(MSG_NULL); Socket->writeByte(1);
						Socket->writeByte(MSG_I_ANALOG);
						Socket->writeShort(p1Input->analog.x);
						Socket->writeShort(p1Input->analog.y);

						packetHandler->SendMsg();

						sendInput.analog.x = p1Input->analog.x;
						sendInput.analog.y = p1Input->analog.y;
					}

					analogTimer = millisecs();
				}
			}
			else if (sendInput.analog.y != 0 || sendInput.analog.x != 0)
			{
				cout << "<< Resetting analog" << endl;
				packetHandler->WriteReliable(); Socket->writeByte(1);
				Socket->writeByte(MSG_I_ANALOG);

				sendInput.analog.y = 0;
				sendInput.analog.x = 0;

				Socket->writeShort(sendInput.analog.x);
				Socket->writeShort(sendInput.analog.y);

				packetHandler->SendMsg(true);
			}
		}
	}
	else
		return;
}
void MemoryHandler::SendPlayer(QSocket* Socket)
{
	player1->read();

	/*
	// If the player still exists while not ingame,
	// re->valuate it to disable writing
	if (remote.system.GameState == GameState::INACTIVE && (player1->Initialized() || player2->Initialized()))
	{
		while (player1->Initialized() || player2->Initialized())
		{
			MemManage::waitFrame();
			player1->pointerEval();
			player2->pointerEval();
		}
	}
	*/

	// If the game has finished loading...
	if (remote.system.GameState >= GameState::LOAD_FINISHED && remote.system.multiMode > 0)
	{
		// Check if the stage has changed so we can re->valuate the player.
		if (local.game.stage != remote.game.stage)
		{
			cout << "<> Stage has changed to " << (ushort)remote.game.stage /*<< "; re->valuating players."*/ << endl;
			
			//player1->pointerEval();
			//player2->pointerEval();
			updateAbstractPlayer(&sendPlayer, player1);

			// Reset the ringcounts so they don't get sent.
			local.game.rings[0] = 0;
			remote.game.rings[0] = 0;
			local.game.rings[1] = 0;

			// Reset specials
			for (int i = 0; i < 3; i++)
				local.game.p2specials[i] = 0;

			// And finally, update the stage so this doesn't loop.
			local.game.stage = remote.game.stage;
		}

		if (CheckTeleport())
		{
			// Send a teleport message
			
			packetHandler->WriteReliable(); Socket->writeByte(2);
			Socket->writeByte(MSG_P_POSITION); Socket->writeByte(MSG_P_SPEED);

			for (int i = 0; i < 3; i++)
			{
				Socket->writeFloat(player1->Position[i]);
				sendPlayer.Position[i] = player1->Position[i];
			}

			Socket->writeFloat(player1->Speed[0]);
			sendPlayer.Speed[0] = player1->Speed[0];

			Socket->writeFloat(player1->Speed[1]);
			sendPlayer.Speed[1] = player1->Speed[1];

			packetHandler->SendMsg(true);
		}

		if (memcmp(&local.game.p1specials[0], &remote.game.p1specials[0], sizeof(char)*3) != 0)
		{
			cout << "<< Sending specials!" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_2PSPECIALS);
			Socket->writeBytes(&remote.game.p1specials[0], 3);
			
			memcpy(&local.game.p1specials[0], &remote.game.p1specials[0], sizeof(char)* 3);

			packetHandler->SendMsg(true);
		}
		if (player1->CharID[0] == 6 || player1->CharID[0] ==  7)
		{
			if (sendPlayer.MechHP != player1->MechHP)
			{
				cout << "<< Sending HP [" << player1->MechHP << "]" << endl;
				packetHandler->WriteReliable(); Socket->writeByte(1);
				Socket->writeByte(MSG_P_HP);
				Socket->writeFloat(player1->MechHP);

				packetHandler->SendMsg(true);
			}
		}

		if (sendPlayer.Action != player1->Action || sendPlayer.Status != player1->Status)
		{
			cout << "<< Sending action...";// << " [S " << sendPlayer.Status << " != " << player1->Status << "]";

			bool sendSpinTimer = ((player1->characterID() == CharacterID::SonicAmy || player1->characterID() == CharacterID::ShadowMetal)
				/*&& (player1->characterID2() == CharacterID2::Sonic || player1->characterID2() == CharacterID2::Shadow)*/);

			if (!isHoldAction(player1->Action))
			{
				cout << endl;
				packetHandler->WriteReliable(); Socket->writeByte((sendSpinTimer) ? 5 : 4);

				Socket->writeByte(MSG_P_POSITION);
				Socket->writeByte(MSG_P_ACTION);
				Socket->writeByte(MSG_P_STATUS);
				Socket->writeByte(MSG_P_ANIMATION);
				
				if (sendSpinTimer)
					Socket->writeByte(MSG_P_SPINTIMER);

				for (int i = 0; i < 3; i++)
					Socket->writeFloat(player1->Position[i]);
				Socket->writeByte(player1->Action);
				Socket->writeShort(player1->Status);
				Socket->writeShort(player1->Animation[0]);

				if (sendSpinTimer)
					Socket->writeShort(player1->SpinTimer);

				packetHandler->SendMsg(true);
			}
			else
			{
				cout << "without status bitfield. SCIENCE!" << endl;
				packetHandler->WriteReliable(); Socket->writeByte(2);

				Socket->writeByte(MSG_P_POSITION);
				Socket->writeByte(MSG_P_ACTION);
				//Socket->writeByte(MSG_P_ANIMATION);

				for (int i = 0; i < 3; i++)
					Socket->writeFloat(player1->Position[i]);
				Socket->writeByte(player1->Action);
					
				//Socket->writeShort(player1->Animation[0]);

				packetHandler->SendMsg(true);
			}
		}

		if (local.game.rings[0] != remote.game.rings[0])
		{
			local.game.rings[0] = remote.game.rings[0];
			cout << "<< Sending rings (" << local.game.rings[0] << ")" << endl;
			Socket->writeByte(MSG_NULL); Socket->writeByte(1);
			Socket->writeByte(MSG_P_RINGS);
			Socket->writeShort(local.game.rings[0]);
			packetHandler->SendMsg();
		}

		if (memcmp(&sendPlayer.Angle[0], &player1->Angle[0], sizeof(float)*3) != 0 || memcmp(&sendPlayer.Speed[0], &player1->Speed[0], sizeof(float)*2) != 0)
		{
			Socket->writeByte(MSG_NULL); Socket->writeByte(3);
		
			Socket->writeByte(MSG_P_ROTATION);
			Socket->writeByte(MSG_P_POSITION);
			Socket->writeByte(MSG_P_SPEED);
			for (int i = 0; i < 3; i++)
				Socket->writeInt(player1->Angle[i]);
			for (int i = 0; i < 3; i++)
				Socket->writeFloat(player1->Position[i]);
			Socket->writeFloat(player1->Speed[0]);
			Socket->writeFloat(player1->Speed[1]);
			Socket->writeFloat(player1->baseSpeed);

			packetHandler->SendMsg();
		}

		if (sendPlayer.Powerups != player1->Powerups)
		{
			cout << "<< Sending powerups" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_P_POWERUPS);
			Socket->writeShort(player1->Powerups);

			packetHandler->SendMsg(true);
		}
		if (sendPlayer.Upgrades != player1->Upgrades)
		{
			cout << "<< Sending upgrades" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_P_UPGRADES);
			Socket->writeInt(player1->Upgrades);

			packetHandler->SendMsg(true);
		}

		updateAbstractPlayer(&sendPlayer, player1);
	}

}
void MemoryHandler::SendMenu(QSocket* Socket)
{
	if (remote.system.GameState == GameState::INACTIVE)
	{
		// Menu analog failsafe
		if (sendInput.analog.x != 0 || sendInput.analog.y != 0)
		{
			cout << "<>\tAnalog failsafe!" << endl;
			p1Input->analog.x = 0;
			p1Input->analog.y = 0;
			sendInput.analog.x = 0;
			sendInput.analog.y = 0;
			p1Input->writeAnalog(&sendInput, 0);
		}

		// ...and we're on the 2P menu...
		if (remote.menu.main == Menu::BATTLE)
		{
			firstMenuEntry = (local.menu.sub != remote.menu.sub);

			if (memcmp(local.menu.battleOpt, remote.menu.battleOpt, sizeof(char)*4) != 0)
			{
				cout << "<< Sending battle options..." << endl;
				memcpy(&local.menu.battleOpt, &remote.menu.battleOpt, sizeof(char)*4);
				local.menu.BattleOptSelection = remote.menu.BattleOptSelection;
				local.menu.BattleOptBack = remote.menu.BattleOptBack;

				packetHandler->WriteReliable(); Socket->writeByte(2);
				Socket->writeByte(MSG_S_BATTLEOPT); Socket->writeByte(MSG_M_BATTLEOPTSEL);

				Socket->writeBytes(&local.menu.battleOpt[0], sizeof(char)*4);
				Socket->writeByte(local.menu.BattleOptSelection);
				Socket->writeByte(local.menu.BattleOptBack);

				packetHandler->SendMsg(true);
			}

			else if (remote.menu.sub == SubMenu2P::S_BATTLEOPT)
			{
				if (local.menu.BattleOptSelection != remote.menu.BattleOptSelection || local.menu.BattleOptBack != remote.menu.BattleOptBack || firstMenuEntry && isServer)
				{
					local.menu.BattleOptSelection = remote.menu.BattleOptSelection;
					local.menu.BattleOptBack = remote.menu.BattleOptBack;

					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_BATTLEOPTSEL);

					Socket->writeByte(local.menu.BattleOptSelection);
					Socket->writeByte(local.menu.BattleOptBack);

					packetHandler->SendMsg(true);
				}
			}


			// ...and we haven't pressed start
			else if (remote.menu.sub == SubMenu2P::S_START && remote.menu.p2start == 0)
			{
				if (!local.menu.atMenu[0])
					local.menu.atMenu[0] = true;

				if (local.menu.atMenu[0] && local.menu.atMenu[1] && !wroteP2Start)
				{
					remote.menu.p2start = 2;
					WriteMemory(ADDR_P2START, &remote.menu.p2start, sizeof(char));
					wroteP2Start = true;
				}
			}
			// ...and we HAVE pressed start
			else if (remote.menu.sub == SubMenu2P::S_READY || remote.menu.sub == SubMenu2P::O_READY)
			{
				if (local.menu.playerReady[0] != remote.menu.playerReady[0])
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_2PREADY);
					Socket->writeByte(remote.menu.playerReady[0]);
					packetHandler->SendMsg(true);

					local.menu.playerReady[0] = remote.menu.playerReady[0];
				}
			}
			else if (remote.menu.sub == SubMenu2P::S_BATTLEMODE || firstMenuEntry && isServer)
			{
				if (local.menu.BattleModeSel != remote.menu.BattleModeSel)
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_BATTLEMODESEL);
					Socket->writeByte(remote.menu.BattleModeSel);
					packetHandler->SendMsg(true);

					local.menu.BattleModeSel = remote.menu.BattleModeSel;
				}
			}
			// Character Selection
			else if (remote.menu.sub == SubMenu2P::S_CHARSEL || remote.menu.sub == SubMenu2P::O_CHARSEL)
			{
				if (remote.menu.selectedChar[0] && remote.menu.selectedChar[1] && remote.menu.sub == SubMenu2P::S_CHARSEL)
				{
					cout << "<> Resetting character selections" << endl;
					ushort temp = 0;
					remote.menu.sub = SubMenu2P::O_CHARSEL;

					WriteMemory(ADDR_CHOSENTIMER, &temp, sizeof(short));
					WriteMemory(ADDR_SUBMENU, &remote.menu.sub, sizeof(int));
				}


				if (local.menu.charSelection[0] != remote.menu.charSelection[0] || firstMenuEntry)
				{
					cout << "<< Sending character selection" << endl;
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_CHARSEL);
					Socket->writeByte(remote.menu.charSelection[0]);
					packetHandler->SendMsg(true);

					local.menu.charSelection[0] = remote.menu.charSelection[0];
				}
				if (local.menu.selectedChar[0] != remote.menu.selectedChar[0])
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_CHARCHOSEN);
					Socket->writeByte(remote.menu.selectedChar[0]);
					packetHandler->SendMsg(true);

					local.menu.selectedChar[0] = remote.menu.selectedChar[0];
				}
				if (memcmp(local.menu.altChar, remote.menu.altChar, 6) != 0 || firstMenuEntry)
				{
					memcpy(&local.menu.altChar, &remote.menu.altChar, 6);

					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_ALTCHAR);

					Socket->writeBytes(&local.menu.altChar[0], 6);

					packetHandler->SendMsg(true);
				}
			}
			else if (remote.menu.sub == SubMenu2P::I_STAGESEL || remote.menu.sub == SubMenu2P::S_STAGESEL)
			{
				if ((memcmp(&local.menu.StageSel2P[0], &remote.menu.StageSel2P[0], (sizeof(int)*2)) != 0 || local.menu.BattleOptButton != remote.menu.BattleOptButton)
					|| firstMenuEntry)
				{
					//cout << "<< Sending Stage Selection" << endl;
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_STAGESEL);
					Socket->writeInt(remote.menu.StageSel2P[0]); Socket->writeInt(remote.menu.StageSel2P[1]);
					Socket->writeByte(remote.menu.BattleOptButton);
					packetHandler->SendMsg(true);

					local.menu.StageSel2P[0] = remote.menu.StageSel2P[0];
					local.menu.StageSel2P[1] = remote.menu.StageSel2P[1];
					local.menu.BattleOptButton = remote.menu.BattleOptButton;
				}
			}
		}
		else
		{
			wroteP2Start = false;
			if (local.menu.atMenu[0])
				local.menu.atMenu[0] = false;
		}

		if (local.menu.atMenu[0] != remote.menu.atMenu[0])
		{
			cout << "<< Sending \"On 2P Menu\" state" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_M_ATMENU);
			Socket->writeByte(local.menu.atMenu[0]);
			packetHandler->SendMsg(true);

			remote.menu.atMenu[0] = local.menu.atMenu[0];
		}

		local.menu.sub = remote.menu.sub;
	}
}
	
void MemoryHandler::Read()
{
	// Check 2P mode and game state
	ReadMemory(ADDR_2PMODE,		&remote.system.multiMode,	sizeof(char));
	ReadMemory(ADDR_GAMESTATE,	&remote.system.GameState,	sizeof(char));

	// Check if the stage has changed
	ReadMemory(ADDR_STAGE, &remote.game.stage, sizeof(char));

	// If the game state is in menu mode, we'll update all the menu stuff
	if (remote.system.GameState == GameState::INACTIVE)
	{
		ReadMemory(ADDR_MENU,		&remote.menu.main,	sizeof(int));
		ReadMemory(ADDR_SUBMENU,	&remote.menu.sub,	sizeof(int));

		if (remote.menu.main == Menu::BATTLE)
		{
			// There must be a better way...
			if (Duration(StartTime) < 5000 && isServer)
			{
				//case SubMenu2P::S_START:
				ReadMemory(ADDR_P2START, &remote.menu.p2start, sizeof(char));

				//case SubMenu2P::S_READY:
				ReadMemory(ADDR_P1READY, &remote.menu.playerReady[0], sizeof(char));

				//case SubMenu2P::S_BATTLEMODE:
				ReadMemory(ADDR_2PMENUSEL, &remote.menu.BattleModeSel, sizeof(char));

				//case SubMenu2P::S_CHARSEL:
				ReadMemory(ADDR_ALTSONIC, &remote.menu.altChar[0], sizeof(char));
				ReadMemory(ADDR_ALTSHADOW, &remote.menu.altChar[1], sizeof(char));

				ReadMemory(ADDR_ALTTAILS, &remote.menu.altChar[2], sizeof(char));
				ReadMemory(ADDR_ALTEGGMAN, &remote.menu.altChar[3], sizeof(char));

				ReadMemory(ADDR_ALTKNUX, &remote.menu.altChar[4], sizeof(char));
				ReadMemory(ADDR_ALTROUGE, &remote.menu.altChar[5], sizeof(char));

				ReadMemory(ADDR_P1CHARSEL, &remote.menu.charSelection[0], sizeof(char));
				ReadMemory(ADDR_P1CHARCHOSEN, &remote.menu.selectedChar[0], sizeof(char));
				ReadMemory(ADDR_P2CHARCHOSEN, &remote.menu.selectedChar[1], sizeof(char));

				//case SubMenu2P::S_STAGESEL:
				ReadMemory(ADDR_STAGESELV, &remote.menu.StageSel2P[0], (sizeof(int)* 2));
				ReadMemory(ADDR_BATTOPT, &remote.menu.battleOpt[0], sizeof(int));
				ReadMemory(ADDR_BATTOPT_BTN, &remote.menu.BattleOptButton, sizeof(char));

				//case SubMenu2P::S_BATTLEOPT:
				ReadMemory(ADDR_BATTOPT, &remote.menu.battleOpt[0], sizeof(int));
				ReadMemory(ADDR_BATTOPT_SEL, &remote.menu.BattleOptSelection, sizeof(char));
				ReadMemory(ADDR_BATTOPT_BAK, &remote.menu.BattleOptBack, sizeof(char));

				return;
			}
			else
			{
				switch(remote.menu.sub)
				{

				case SubMenu2P::S_START:
					ReadMemory(ADDR_P2START, &remote.menu.p2start, sizeof(char));
					return;

				case SubMenu2P::S_READY:
					ReadMemory(ADDR_P1READY, &remote.menu.playerReady[0], sizeof(char));
					return;

				case SubMenu2P::S_BATTLEMODE:
					ReadMemory(ADDR_2PMENUSEL, &remote.menu.BattleModeSel, sizeof(char));
					return;

				case SubMenu2P::S_CHARSEL:
					// ohhh boy...
					// This monstrosity reads all the alt character menu info
					ReadMemory(ADDR_ALTSONIC,	&remote.menu.altChar[0], sizeof(char));
					ReadMemory(ADDR_ALTSHADOW,	&remote.menu.altChar[1], sizeof(char));

					ReadMemory(ADDR_ALTTAILS,	&remote.menu.altChar[2], sizeof(char));
					ReadMemory(ADDR_ALTEGGMAN,	&remote.menu.altChar[3], sizeof(char));

					ReadMemory(ADDR_ALTKNUX,	&remote.menu.altChar[4], sizeof(char));
					ReadMemory(ADDR_ALTROUGE,	&remote.menu.altChar[5], sizeof(char));

					ReadMemory(ADDR_P1CHARSEL,		&remote.menu.charSelection[0], sizeof(char));
					ReadMemory(ADDR_P1CHARCHOSEN,	&remote.menu.selectedChar[0], sizeof(char));
					ReadMemory(ADDR_P2CHARCHOSEN,	&remote.menu.selectedChar[1], sizeof(char));

					return;

				case SubMenu2P::S_STAGESEL:
					ReadMemory(ADDR_STAGESELV,	&remote.menu.StageSel2P[0], (sizeof(int)*2));
					ReadMemory(ADDR_BATTOPT,	&remote.menu.battleOpt[0], sizeof(int));
					ReadMemory(ADDR_BATTOPT_BTN, &remote.menu.BattleOptButton, sizeof(char));
					return;

				case SubMenu2P::S_BATTLEOPT:
					ReadMemory(ADDR_BATTOPT,	&remote.menu.battleOpt[0], sizeof(char) * 4);
					ReadMemory(ADDR_BATTOPT_SEL, &remote.menu.BattleOptSelection, sizeof(char));
					ReadMemory(ADDR_BATTOPT_BAK, &remote.menu.BattleOptBack, sizeof(char));
					return;

				default:
					return;
				}
			}
		}
	}
	else if (remote.system.GameState >= GameState::INGAME)
	{
		ReadMemory(ADDR_P1RINGS, &remote.game.rings[0], sizeof(short));
		ReadMemory(ADDR_P1SPECIALS, &remote.game.p1specials[0], (sizeof(char)*3));
		ReadMemory(ADDR_TIMESTOP, &remote.game.TimeStop, sizeof(char));
		ReadMemory(ADDR_PLYPAUSED, &remote.system.PlayerPaused, sizeof(char));
		ReadMemory(ADDR_TIME, &remote.game.Time, sizeof(char)* 3);

		if (remote.system.GameState == GameState::PAUSE)
			ReadMemory(ADDR_PAUSESEL, &remote.system.PauseMenuSelection, sizeof(char));

		return;
	}
}
inline void MemoryHandler::writeP2Memory()
{
	if (remote.system.GameState >= GameState::INGAME)
		player2->write(&recvPlayer);
}
inline void MemoryHandler::writeRings() { RingCount[1] = local.game.rings[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.p2specials, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStop; }

void MemoryHandler::updateAbstractPlayer(AbstractPlayer* recvr, PlayerObject* player)
{
	// Mech synchronize hack
	player2->MechHP		= recvPlayer.MechHP;
	player2->Powerups	= recvPlayer.Powerups;
	player2->Upgrades	= recvPlayer.Upgrades;


	memcpy(recvr, &player->Action, sizeof(AbstractPlayer));
}

void MemoryHandler::ToggleSplitscreen()
{
	if (remote.system.GameState == GameState::INGAME && remote.system.multiMode > 0)
	{
		if ((sendInput.buttons.Held & (1 << 16)) && (sendInput.buttons.Held & (2 << 16)))
		{
			if (!splitToggled)
			{
				ReadMemory(ADDR_SPLITSCREEN, &remote.system.splitscreen, sizeof(char));

				if (remote.system.splitscreen == 1)
					remote.system.splitscreen = 2;
				else if (remote.system.splitscreen == 2)
					remote.system.splitscreen = 1;
				else
					return;

				WriteMemory(ADDR_SPLITSCREEN, &remote.system.splitscreen, sizeof(char));
				splitToggled = true;
			}
		}

		else if (splitToggled)
			splitToggled = false;
	}

	return;
}
bool MemoryHandler::CheckTeleport()
{
	if (remote.system.GameState == GameState::INGAME && remote.system.multiMode > 0)
	{
		if ((sendInput.buttons.Held & (1 << 5)) && (sendInput.buttons.Held & (1 << 9)))
		{
			if (!Teleported)
			{
				// Teleport to recvPlayer
				cout << "<> Teleporting to other player..." << endl;;
				player1->Teleport(&recvPlayer);

				return Teleported = true;
			}
		}
		else if (Teleported)
			return Teleported = false;
	}

	return false;
}

void MemoryHandler::ReceiveInput(QSocket* Socket, uchar type)
{
	if (remote.menu.main == 16 || remote.system.multiMode > 0 && remote.system.GameState > GameState::INACTIVE)
	{
		switch(type)
		{
			default:
				return;

			case MSG_I_BUTTONS:
				recvInput.buttons.Held = Socket->readInt();
				if (CheckFrame())
					MemManage::waitFrame(1, thisFrame);
				p2Input->writeButtons(&recvInput);

				return;

			case MSG_I_ANALOG:
				//cout << ">> Received analog!" << endl;
				recvInput.analog.x = Socket->readShort();
				recvInput.analog.y = Socket->readShort();

				p2Input->writeAnalog(&recvInput, remote.system.GameState);

				return;
		}
	}
	else
		return;
}

void MemoryHandler::ReceiveSystem(QSocket* Socket, uchar type)
{
	if (remote.system.GameState >= GameState::LOAD_FINISHED)
	{
		switch (type)
		{
			default:
				return;

			case MSG_S_TIME:
				for (int i = 0; i < 3; i++)
					local.game.Time[i] = (char)Socket->readByte();
				WriteMemory(ADDR_TIME, &local.game.Time[0], sizeof(char)* 3);
				return;

			case MSG_S_GAMESTATE:
			{
				uchar recvGameState = (char)Socket->readByte();

				if (remote.system.GameState >= GameState::INGAME && recvGameState > GameState::LOAD_FINISHED)
					MemManage::changeGameState(recvGameState, &local);

				return;
			}

			case MSG_S_PAUSESEL:
				local.system.PauseMenuSelection = (uchar)Socket->readByte();
				WriteMemory(ADDR_PAUSESEL, &local.system.PauseMenuSelection, sizeof(char));
				return;

			case MSG_S_TIMESTOP:
				local.game.TimeStop = (char)Socket->readByte();
				writeTimeStop();
				return;

			case MSG_2PSPECIALS:
				for (int i = 0; i < 3; i++)
					local.game.p2specials[i] = (char)Socket->readByte();

				writeSpecials();
				return;

			case MSG_P_RINGS:
				local.game.rings[1] = Socket->readShort();
				writeRings();

				cout << ">> Ring Count Change: " << local.game.rings[1] << endl;
				return;
		}
	}
}

void MemoryHandler::ReceivePlayer(QSocket* Socket, uchar type)
{
	if (remote.system.GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = false;
		switch(type)
		{
			default:
				return;

			case MSG_P_HP:
				recvPlayer.MechHP = Socket->readFloat();
				cout << ">> Received HP update. (" << recvPlayer.MechHP << ")" << endl;
				writePlayer = true;
				break;

			case MSG_P_ACTION:
				recvPlayer.Action = Socket->readChar();
				writePlayer = true;
				break;

			case MSG_P_STATUS:
				recvPlayer.Status = Socket->readShort();
				writePlayer = true;
				break;

			case MSG_P_SPINTIMER:
				recvPlayer.SpinTimer = Socket->readShort();
				writePlayer = true;
				break;

			case MSG_P_ANIMATION:
				recvPlayer.Animation[0] = Socket->readShort();
				writePlayer = true;
				break;

			case MSG_P_POSITION:
				for (ushort i = 0; i < 3; i++)
					recvPlayer.Position[i] = Socket->readFloat();
				writePlayer = true;
				break;

			case MSG_P_ROTATION:
				for (ushort i = 0; i < 3; i++)
					recvPlayer.Angle[i] = Socket->readInt();
				writePlayer = true;
				break;

			case MSG_P_SPEED:
				recvPlayer.Speed[0]		= Socket->readFloat();
				recvPlayer.Speed[1]		= Socket->readFloat();
				recvPlayer.baseSpeed	= Socket->readFloat();

				writePlayer = true;
				break;

			case MSG_P_POWERUPS:
				recvPlayer.Powerups = Socket->readShort();
				writePlayer = true;
				break;

			case MSG_P_UPGRADES:
				recvPlayer.Upgrades = Socket->readInt();
				writePlayer = true;
				break;
		}
			
		if (writePlayer)
			writeP2Memory();

		return;
	}
	else
		return;
}
void MemoryHandler::ReceiveMenu(QSocket* Socket, uchar type)
{
	if (remote.system.GameState == GameState::INACTIVE)
	{
		switch(type)
		{
			default:
				return;

			case MSG_M_ATMENU:
				local.menu.atMenu[1] = Socket->readOct();
				if (local.menu.atMenu[1])
					cout << ">> Player 2 is ready on the 2P menu!" << endl;
				else
					cout << ">> Player 2 is no longer on the 2P menu." << endl;
				return;

			case MSG_2PREADY:
				local.menu.playerReady[1] = (uchar)Socket->readByte();

				WriteMemory(ADDR_P2READY, &local.menu.playerReady[1], sizeof(char));
				cout << ">> Player 2 ready state changed." << endl;
				return;

			case MSG_M_CHARSEL:
				local.menu.charSelection[1] = (uchar)Socket->readByte();

				WriteMemory(ADDR_P2CHARSEL, &local.menu.charSelection[1], sizeof(char));
				return;

			case MSG_M_CHARCHOSEN:
				local.menu.selectedChar[1] = (Socket->readOct() == 1) ? true : false;

				WriteMemory(ADDR_P2CHARCHOSEN, &local.menu.selectedChar[1], sizeof(char));
				return;

			case MSG_M_ALTCHAR:
				for (int i = 0; i < 6; i++)
					local.menu.altChar[i] = (char)Socket->readByte();

				WriteMemory(ADDR_ALTSONIC, &local.menu.altChar[0], sizeof(char));
				WriteMemory(ADDR_ALTSHADOW, &local.menu.altChar[1], sizeof(char));

				WriteMemory(ADDR_ALTTAILS, &local.menu.altChar[2], sizeof(char));
				WriteMemory(ADDR_ALTEGGMAN, &local.menu.altChar[3], sizeof(char));

				WriteMemory(ADDR_ALTKNUX, &local.menu.altChar[4], sizeof(char));
				WriteMemory(ADDR_ALTROUGE, &local.menu.altChar[5], sizeof(char));

				return;

			case MSG_S_BATTLEOPT:
				for (int i = 0; i < 4; i++)
					local.menu.battleOpt[i] = (char)Socket->readByte();
				WriteMemory(ADDR_BATTOPT, &local.menu.battleOpt[0], (sizeof(char)*4));

				return;

			case MSG_M_BATTLEOPTSEL:
				local.menu.BattleOptSelection = (char)Socket->readByte();
				local.menu.BattleOptBack = (char)Socket->readByte();

				WriteMemory(ADDR_BATTOPT_SEL, &local.menu.BattleOptSelection, sizeof(char));
				WriteMemory(ADDR_BATTOPT_BAK, &local.menu.BattleOptBack, sizeof(char));

				return;

			case MSG_M_STAGESEL:
				local.menu.StageSel2P[0] = Socket->readInt();
				local.menu.StageSel2P[1] = Socket->readInt();
				local.menu.BattleOptButton = (char)Socket->readByte();

				WriteMemory(ADDR_STAGESELV, &local.menu.StageSel2P[0], (sizeof(int)*2));
				WriteMemory(ADDR_BATTOPT_BTN, &local.menu.BattleOptButton, sizeof(char));

				return;

			case MSG_M_BATTLEMODESEL:
				local.menu.BattleModeSel = (char)Socket->readByte();
				WriteMemory(ADDR_2PMENUSEL, &local.menu.BattleModeSel, sizeof(char));

				return;
		}
	}
	else
		return;
}

void MemoryHandler::PreReceive()
{
	player2->read();
	updateAbstractPlayer(&recvPlayer, player2);
	
	p2Input->read();

	writeRings();
	writeSpecials();

	return;
}

void MemoryHandler::PostReceive()
{
	player2->read();
	updateAbstractPlayer(&recvPlayer, player2);
	writeP2Memory();

	p2Input->read();

	writeRings();
	writeSpecials();

	return;
}