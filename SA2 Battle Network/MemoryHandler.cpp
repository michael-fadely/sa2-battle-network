#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LazyMemory.h"

#include "Common.h"
#include "CommonEnums.h"
#include "AddressList.h"
#include "ActionBlacklist.h"

#include "MemoryManagement.h"
#include "NewPlayerObject.h"
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

	cAt2PMenu[0] = false;
	cAt2PMenu[1] = false;
	lAt2PMenu[0] = false;
	lAt2PMenu[1] = false;

	firstMenuEntry = false;
	wroteP2Start = false;
	splitToggled = false;
	Teleported = false;
	writePlayer = false;

	//MainCharacter[0] = nullptr;
	//MainCharacter[1] = nullptr;
	p1Input = nullptr;
	p2Input = nullptr;

	//InitPlayers();
	InitInput();

	local = {};
	//recvPlayer = {};
	//sendPlayer = {};
	recvInput = {};
	sendInput = {};

	thisFrame = 0;
	lastFrame = 0;

	return;
}
MemoryHandler::~MemoryHandler()
{
	packetHandler = nullptr;
	cout << "<> [MemoryHandler::~MemoryHandler] Deinitializing Player Objects and Input Structures" << endl;
	//DeinitPlayers();
	DeinitInput();
}

/*
void MemoryHandler::InitPlayers()
{
	if (MainCharacter[0] == nullptr)
		MainCharacter[0] = new PlayerObject(ADDR_MainCharacter[0]);
	else
		cout << "<> [MemoryHandler::InitPlayers] Player 1 has already been initialized." << endl;

	if (MainCharacter[1] == nullptr)
		MainCharacter[1] = new PlayerObject(ADDR_MainCharacter[1]);
	else
		cout << "<> [MemoryHandler::InitPlayers] Player 2 has already been initialized." << endl;

	return;
}
void MemoryHandler::DeinitPlayers()
{
	if (MainCharacter[0] != nullptr)
	{
		delete MainCharacter[0];
		MainCharacter[0] = nullptr;
	}
	if (MainCharacter[1] != nullptr)
	{
		delete MainCharacter[1];
		MainCharacter[1] = nullptr;
	}

	return;
}
*/

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
	if (GameState >= GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		if (local.system.GameState != GameState && GameState > GameState::LOAD_FINISHED)
		{
			cout << "<< Sending gamestate [" << (ushort)GameState << "]" << endl;

			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_GAMESTATE);
			Socket->writeByte(GameState);

			packetHandler->SendMsg(true);
			local.system.GameState = GameState;
		}

		if (local.system.PauseSelection != PauseSelection)
		{
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_PAUSESEL);
			Socket->writeByte(PauseSelection);

			packetHandler->SendMsg(true);
			local.system.PauseSelection = PauseSelection;
		}

		if (local.game.TimerSeconds != TimerSeconds && isServer)
		{
			Socket->writeByte(MSG_NULL); Socket->writeByte(2);
			Socket->writeByte(MSG_S_TIME);
			Socket->writeByte(MSG_KEEPALIVE);

			Socket->writeByte(TimerMinutes);
			Socket->writeByte(TimerSeconds);
			Socket->writeByte(TimerFrames);

			packetHandler->SendMsg();
			memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			packetHandler->setSentKeepalive();
		}

		if (local.game.TimeStopMode != TimeStopMode)
		{
			cout << "<< Sending time stop [";
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_S_TIMESTOP);

			// Swap the value since player 1 is relative to the client
			switch (TimeStopMode)
			{
			default:
			case 0:
				cout << 0;
				Socket->writeByte(0);
				break;

			case 1:
				cout << 2;
				Socket->writeByte(2);
				break;

			case 2:
				cout << 1;
				Socket->writeByte(1);
				break;
			}
			
			cout << "]" << endl;
			packetHandler->SendMsg(true);
			local.game.TimeStopMode = TimeStopMode;
		}
	}
	if (GameState != GameState::INGAME || TimeStopMode > 0 || !isServer)
	{
		if (Duration(packetHandler->getSentKeepalive()) >= 1000)
		{
			Socket->writeByte(MSG_NULL); Socket->writeByte(1);
			Socket->writeByte(MSG_KEEPALIVE);

			packetHandler->SendMsg();
			packetHandler->setSentKeepalive();
		}
	}
}
void MemoryHandler::SendInput(QSocket* Socket, uint sendTimer)
{
	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		p1Input->read();

		if (!CheckFrame())
			ToggleSplitscreen();

		if (sendInput.buttons.Held != p1Input->buttons.Held && GameState != GameState::MEMCARD)
		{
			packetHandler->WriteReliable(); Socket->writeByte(1);

			Socket->writeByte(MSG_I_BUTTONS);
			Socket->writeInt(p1Input->buttons.Held);

			packetHandler->SendMsg(true);

			sendInput.buttons.Held = p1Input->buttons.Held;

		}

		if (sendInput.analog.x != p1Input->analog.x || sendInput.analog.y != p1Input->analog.y)
		{
			if (GameState == GameState::INGAME)
			{
				if (Duration(analogTimer) >= 125)
				{
					if (p1Input->analog.x == 0 && p1Input->analog.y == 0)
					{
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
	// If the game has finished loading...
	if (GameState >= GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		// Check if the stage has changed so we can re->valuate the player.
		if (local.game.CurrentLevel != CurrentLevel)
		{
			cout << "<> Stage has changed to " << (ushort)CurrentLevel << endl;

			updateAbstractPlayer(&sendPlayer, MainCharacter[0]);

			// Reset the ringcounts so they don't get sent.
			local.game.RingCount[0] = 0;
			RingCount[0] = 0;
			local.game.RingCount[1] = 0;

			// Reset specials
			for (int i = 0; i < 3; i++)
				local.game.P2SpecialAttacks[i] = 0;

			// And finally, update the stage so this doesn't loop.
			local.game.CurrentLevel = CurrentLevel;
		}

		if (CheckTeleport())
		{
			// Send a teleport message

			packetHandler->WriteReliable(); Socket->writeByte(2);
			Socket->writeByte(MSG_P_POSITION); Socket->writeByte(MSG_P_SPEED);

			Socket->writeFloat(MainCharacter[0]->Data1->Position.x);
			Socket->writeFloat(MainCharacter[0]->Data1->Position.y);
			Socket->writeFloat(MainCharacter[0]->Data1->Position.z);
			
			sendPlayer.Data1.Position = MainCharacter[0]->Data1->Position;

			Socket->writeFloat(MainCharacter[0]->Data2->HSpeed);
			sendPlayer.Data2.HSpeed = MainCharacter[0]->Data2->HSpeed;

			Socket->writeFloat(MainCharacter[0]->Data2->VSpeed);
			sendPlayer.Data2.VSpeed = MainCharacter[0]->Data2->VSpeed;

			packetHandler->SendMsg(true);
		}

		if (memcmp(&local.game.P1SpecialAttacks[0], &P1SpecialAttacks[0], sizeof(char) * 3) != 0)
		{
			cout << "<< Sending specials!" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_2PSPECIALS);
			Socket->writeBytes(&P1SpecialAttacks[0], 3);

			memcpy(&local.game.P1SpecialAttacks[0], &P1SpecialAttacks[0], sizeof(char) * 3);

			packetHandler->SendMsg(true);
		}
		if (MainCharacter[0]->Data2->CharID == 6 || MainCharacter[0]->Data2->CharID == 7)
		{
			if (sendPlayer.Data2.MechHP != MainCharacter[0]->Data2->MechHP)
			{
				cout << "<< Sending HP [" << MainCharacter[0]->Data2->MechHP << "]" << endl;
				packetHandler->WriteReliable(); Socket->writeByte(1);
				Socket->writeByte(MSG_P_HP);
				Socket->writeFloat(MainCharacter[0]->Data2->MechHP);
				
				packetHandler->SendMsg(true);
			}
		}

		if (sendPlayer.Data1.Action != MainCharacter[0]->Data1->Action || sendPlayer.Data1.Status != MainCharacter[0]->Data1->Status)
		{
			cout << "<< Sending action...";

			bool sendSpinTimer = (MainCharacter[0]->Data2->CharID2 == Characters_Sonic || MainCharacter[0]->Data2->CharID2 == Characters_Sonic);

			if (!isHoldAction(MainCharacter[0]->Data1->Action))
			{
				cout << endl;
				packetHandler->WriteReliable(); Socket->writeByte((sendSpinTimer) ? 5 : 4);

				Socket->writeByte(MSG_P_POSITION);
				Socket->writeByte(MSG_P_ACTION);
				Socket->writeByte(MSG_P_STATUS);
				Socket->writeByte(MSG_P_ANIMATION);

				if (sendSpinTimer)
					Socket->writeByte(MSG_P_SPINTIMER);

				Socket->writeFloat(MainCharacter[0]->Data1->Position.x);
				Socket->writeFloat(MainCharacter[0]->Data1->Position.y);
				Socket->writeFloat(MainCharacter[0]->Data1->Position.z);

				Socket->writeByte(MainCharacter[0]->Data1->Action);
				Socket->writeShort(MainCharacter[0]->Data1->Status);
				Socket->writeShort(MainCharacter[0]->Data2->AnimInfo.Next);

				if (sendSpinTimer)
					Socket->writeShort(((SonicCharObj2*)MainCharacter[0]->Data2)->SpindashTimer);

				packetHandler->SendMsg(true);
			}
			else
			{
				cout << "without status bitfield. SCIENCE!" << endl;
				packetHandler->WriteReliable(); Socket->writeByte(2);

				Socket->writeByte(MSG_P_POSITION);
				Socket->writeByte(MSG_P_ACTION);

				Socket->writeFloat(MainCharacter[0]->Data1->Position.x);
				Socket->writeFloat(MainCharacter[0]->Data1->Position.y);
				Socket->writeFloat(MainCharacter[0]->Data1->Position.z);
				Socket->writeByte(MainCharacter[0]->Data1->Action);

				packetHandler->SendMsg(true);
			}
		}

		if (local.game.RingCount[0] != RingCount[0])
		{
			local.game.RingCount[0] = RingCount[0];
			cout << "<< Sending rings (" << local.game.RingCount[0] << ")" << endl;
			Socket->writeByte(MSG_NULL); Socket->writeByte(1);
			Socket->writeByte(MSG_P_RINGS);
			Socket->writeShort(local.game.RingCount[0]);
			packetHandler->SendMsg();
		}

		if (memcmp(&sendPlayer.Data1.Rotation, &MainCharacter[0]->Data1->Rotation, sizeof(float) * 3) != 0 || memcmp(&sendPlayer.Data2.HSpeed, &MainCharacter[0]->Data2->HSpeed, sizeof(float) * 2) != 0)
		{
			Socket->writeByte(MSG_NULL); Socket->writeByte(3);

			Socket->writeByte(MSG_P_ROTATION);
			Socket->writeByte(MSG_P_POSITION);
			Socket->writeByte(MSG_P_SPEED);
			Socket->writeInt(MainCharacter[0]->Data1->Rotation.x);
			Socket->writeInt(MainCharacter[0]->Data1->Rotation.y);
			Socket->writeInt(MainCharacter[0]->Data1->Rotation.z);
			Socket->writeFloat(MainCharacter[0]->Data1->Position.x);
			Socket->writeFloat(MainCharacter[0]->Data1->Position.y);
			Socket->writeFloat(MainCharacter[0]->Data1->Position.z);
			Socket->writeFloat(MainCharacter[0]->Data2->HSpeed);
			Socket->writeFloat(MainCharacter[0]->Data2->VSpeed);
			Socket->writeFloat(MainCharacter[0]->Data2->PhysData.BaseSpeed);

			packetHandler->SendMsg();
		}

		if (sendPlayer.Data2.Powerups != MainCharacter[0]->Data2->Powerups)
		{
			cout << "<< Sending powerups" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_P_POWERUPS);
			Socket->writeShort(MainCharacter[0]->Data2->Powerups);

			packetHandler->SendMsg(true);
		}
		if (sendPlayer.Data2.Upgrades != MainCharacter[0]->Data2->Upgrades)
		{
			cout << "<< Sending upgrades" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_P_UPGRADES);
			Socket->writeInt(MainCharacter[0]->Data2->Upgrades);

			packetHandler->SendMsg(true);
		}

		updateAbstractPlayer(&sendPlayer, MainCharacter[0]);
	}

}
void MemoryHandler::SendMenu(QSocket* Socket)
{
	if (GameState == GameState::INACTIVE)
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
		if (CurrentMenu[0] == Menu::BATTLE)
		{
			firstMenuEntry = (local.menu.sub != CurrentMenu[1]);

			if (memcmp(local.menu.BattleOptions, BattleOptions, sizeof(char) * 4) != 0)
			{
				cout << "<< Sending battle options..." << endl;
				memcpy(&local.menu.BattleOptions, BattleOptions, sizeof(char) * 4);
				local.menu.BattleOptionsSelection = BattleOptionsSelection;
				local.menu.BattleOptionsBackSelected = BattleOptionsBackSelected;

				packetHandler->WriteReliable(); Socket->writeByte(2);
				Socket->writeByte(MSG_S_BATTLEOPT); Socket->writeByte(MSG_M_BATTLEOPTSEL);

				Socket->writeBytes(&local.menu.BattleOptions[0], sizeof(char) * 4);
				Socket->writeByte(local.menu.BattleOptionsSelection);
				Socket->writeByte(local.menu.BattleOptionsBackSelected);

				packetHandler->SendMsg(true);
			}

			else if (CurrentMenu[1] == SubMenu2P::S_BATTLEOPT)
			{
				if (local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBackSelected != BattleOptionsBackSelected || firstMenuEntry && isServer)
				{
					local.menu.BattleOptionsSelection = BattleOptionsSelection;
					local.menu.BattleOptionsBackSelected = BattleOptionsBackSelected;

					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_BATTLEOPTSEL);

					Socket->writeByte(local.menu.BattleOptionsSelection);
					Socket->writeByte(local.menu.BattleOptionsBackSelected);

					packetHandler->SendMsg(true);
				}
			}


			// ...and we haven't pressed start
			else if (cAt2PMenu[0] = (CurrentMenu[1] == SubMenu2P::S_START && P2Start == 0))
			{
				if (cAt2PMenu[0] && cAt2PMenu[1] && !wroteP2Start)
				{
					P2Start = 2;
					wroteP2Start = true;
				}
			}
			// ...and we HAVE pressed start
			else if (CurrentMenu[1] == SubMenu2P::S_READY || CurrentMenu[1] == SubMenu2P::O_READY)
			{
				if (local.menu.PlayerReady[0] != PlayerReady[0])
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_2PREADY);
					Socket->writeByte(PlayerReady[0]);
					packetHandler->SendMsg(true);

					local.menu.PlayerReady[0] = PlayerReady[0];
				}
			}
			else if (CurrentMenu[1] == SubMenu2P::S_BATTLEMODE || firstMenuEntry && isServer)
			{
				if (local.menu.BattleSelection != BattleSelection)
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_BATTLEMODESEL);
					Socket->writeByte(BattleSelection);
					packetHandler->SendMsg(true);

					local.menu.BattleSelection = BattleSelection;
				}
			}
			// Character Selection
			else if (CurrentMenu[1] == SubMenu2P::S_CHARSEL || CurrentMenu[1] == SubMenu2P::O_CHARSEL)
			{
				if (CharacterSelected[0] && CharacterSelected[1] && CurrentMenu[1] == SubMenu2P::S_CHARSEL)
				{
					cout << "<> Resetting character selections" << endl;
					CharacterSelectTimer = 0;
					CurrentMenu[1] = SubMenu2P::O_CHARSEL;
				}


				if (local.menu.CharacterSelection[0] != CharacterSelection[0] || firstMenuEntry)
				{
					cout << "<< Sending character selection" << endl;
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_CHARSEL);
					Socket->writeByte(CharacterSelection[0]);
					packetHandler->SendMsg(true);

					local.menu.CharacterSelection[0] = CharacterSelection[0];
				}
				if (local.menu.CharacterSelected[0] != CharacterSelected[0])
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_CHARCHOSEN);
					Socket->writeByte(CharacterSelected[0]);
					packetHandler->SendMsg(true);

					local.menu.CharacterSelected[0] = CharacterSelected[0];
				}
				if (firstMenuEntry ||
					((local.menu.AltCharacterSonic != AltCharacterSonic) ||
					(local.menu.AltCharacterShadow != AltCharacterShadow) ||
					(local.menu.AltCharacterTails != AltCharacterTails) ||
					(local.menu.AltCharacterEggman != AltCharacterEggman) ||
					(local.menu.AltCharacterKnuckles != AltCharacterKnuckles) ||
					(local.menu.AltCharacterRouge != AltCharacterRouge))
					)
				{
					local.menu.AltCharacterSonic = AltCharacterSonic;
					local.menu.AltCharacterShadow = AltCharacterShadow;
					local.menu.AltCharacterTails = AltCharacterTails;
					local.menu.AltCharacterEggman = AltCharacterEggman;
					local.menu.AltCharacterKnuckles = AltCharacterKnuckles;
					local.menu.AltCharacterRouge = AltCharacterRouge;

					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_ALTCHAR);

					Socket->writeBytes(&local.menu.AltCharacterSonic, sizeof(char) * 6);

					packetHandler->SendMsg(true);
				}
			}
			else if (CurrentMenu[1] == SubMenu2P::I_STAGESEL || CurrentMenu[1] == SubMenu2P::S_STAGESEL)
			{
				if ((memcmp(&local.menu.StageSelection2P[0], &StageSelection2P[0], (sizeof(int) * 2)) != 0 || local.menu.BattleOptionsButton != BattleOptionsButton)
					|| firstMenuEntry)
				{
					packetHandler->WriteReliable(); Socket->writeByte(1);
					Socket->writeByte(MSG_M_STAGESEL);
					Socket->writeInt(StageSelection2P[0]); Socket->writeInt(StageSelection2P[1]);
					Socket->writeByte(BattleOptionsButton);
					packetHandler->SendMsg(true);

					local.menu.StageSelection2P[0] = StageSelection2P[0];
					local.menu.StageSelection2P[1] = StageSelection2P[1];
					local.menu.BattleOptionsButton = BattleOptionsButton;
				}
			}
		}
		else
		{
			wroteP2Start = false;
			if (cAt2PMenu[0])
				cAt2PMenu[0] = false;
		}

		if (cAt2PMenu[0] != lAt2PMenu[0])
		{
			cout << "<< Sending \"On 2P Menu\" state" << endl;
			packetHandler->WriteReliable(); Socket->writeByte(1);
			Socket->writeByte(MSG_M_ATMENU);
			Socket->writeByte(cAt2PMenu[0]);
			packetHandler->SendMsg(true);

			lAt2PMenu[0] = cAt2PMenu[0];
		}

		local.menu.sub = CurrentMenu[1];
	}
}

inline void MemoryHandler::writeP2Memory()
{
	if (GameState >= GameState::INGAME)
		PlayerObject::WritePlayer(MainCharacter[1], &recvPlayer);
}
inline void MemoryHandler::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

void MemoryHandler::updateAbstractPlayer(PlayerObject* recvr, ObjectMaster* player)
{
	// Mech synchronize hack
	if (GameState >= GameState::INGAME && MainCharacter[1] != nullptr)
	{
		if (MainCharacter[1]->Data2->CharID2 == Characters_MechEggman || MainCharacter[1]->Data2->CharID2 == Characters_MechTails)
			MainCharacter[1]->Data2->MechHP = recvPlayer.Data2.MechHP;
		MainCharacter[1]->Data2->Powerups = recvPlayer.Data2.Powerups;
		MainCharacter[1]->Data2->Upgrades = recvPlayer.Data2.Upgrades;
	}

	recvr->Set(player);
}

void MemoryHandler::ToggleSplitscreen()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((sendInput.buttons.Held & (1 << 16)) && (sendInput.buttons.Held & (2 << 16)))
		{
			if (!splitToggled)
			{
				if (SplitscreenMode == 1)
					SplitscreenMode = 2;
				else if (SplitscreenMode == 2)
					SplitscreenMode = 1;
				else
					return;
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
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((sendInput.buttons.Held & (1 << 5)) && (sendInput.buttons.Held & (1 << 9)))
		{
			if (!Teleported)
			{
				// Teleport to recvPlayer
				cout << "<> Teleporting to other player..." << endl;;
				//MainCharacter[0]->Teleport(&recvPlayer);

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
	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
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
			recvInput.analog.x = Socket->readShort();
			recvInput.analog.y = Socket->readShort();

			p2Input->writeAnalog(&recvInput, GameState);

			return;
		}
	}
	else
		return;
}

void MemoryHandler::ReceiveSystem(QSocket* Socket, uchar type)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		switch (type)
		{
		default:
			return;

		case MSG_S_TIME:
			TimerMinutes = local.game.TimerMinutes = Socket->readChar();
			TimerSeconds = local.game.TimerSeconds = Socket->readChar();
			TimerFrames = local.game.TimerFrames = Socket->readChar();
			return;

		case MSG_S_GAMESTATE:
		{
			uchar recvGameState = Socket->readChar();

			if (GameState >= GameState::INGAME && recvGameState > GameState::LOAD_FINISHED)
				MemManage::changeGameState(recvGameState, &local);

			return;
		}

		case MSG_S_PAUSESEL:
			PauseSelection = local.system.PauseSelection = (uchar)Socket->readChar();
			return;

		case MSG_S_TIMESTOP:
			local.game.TimeStopMode = Socket->readChar();
			writeTimeStop();
			return;

		case MSG_2PSPECIALS:
			for (int i = 0; i < 3; i++)
				local.game.P2SpecialAttacks[i] = Socket->readChar();

			writeSpecials();
			return;

		case MSG_P_RINGS:
			local.game.RingCount[1] = Socket->readShort();
			writeRings();

			cout << ">> Ring Count Change: " << local.game.RingCount[1] << endl;
			return;
		}
	}
}

void MemoryHandler::ReceivePlayer(QSocket* Socket, uchar type)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = false;
		switch (type)
		{
		default:
			return;

		case MSG_P_HP:
			recvPlayer.Data2.MechHP = Socket->readFloat();
			cout << ">> Received HP update. (" << recvPlayer.Data2.MechHP << ")" << endl;
			writePlayer = true;
			break;

		case MSG_P_ACTION:
			recvPlayer.Data1.Action = Socket->readChar();
			writePlayer = true;
			break;

		case MSG_P_STATUS:
			recvPlayer.Data1.Status = Socket->readShort();
			writePlayer = true;
			break;

		case MSG_P_SPINTIMER:
			recvPlayer.Sonic.SpindashTimer = Socket->readShort();
			writePlayer = true;
			break;

		case MSG_P_ANIMATION:
			recvPlayer.Data2.AnimInfo.Next = Socket->readShort();
			writePlayer = true;
			break;

		case MSG_P_POSITION:
			recvPlayer.Data1.Position.x = Socket->readFloat();
			recvPlayer.Data1.Position.z = Socket->readFloat();
			recvPlayer.Data1.Position.y = Socket->readFloat();
			writePlayer = true;
			break;

		case MSG_P_ROTATION:
			recvPlayer.Data1.Rotation.x = Socket->readInt();
			recvPlayer.Data1.Rotation.y = Socket->readInt();
			recvPlayer.Data1.Rotation.z = Socket->readInt();
			writePlayer = true;
			break;

		case MSG_P_SPEED:
			recvPlayer.Data2.HSpeed = Socket->readFloat();
			recvPlayer.Data2.VSpeed = Socket->readFloat();
			recvPlayer.Data2.PhysData.BaseSpeed = Socket->readFloat();

			writePlayer = true;
			break;

		case MSG_P_POWERUPS:
			recvPlayer.Data2.Powerups = (Powerups)Socket->readShort();
			writePlayer = true;
			break;

		case MSG_P_UPGRADES:
			recvPlayer.Data2.Upgrades = (Upgrades)Socket->readInt();
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
	if (GameState == GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return;

		case MSG_M_ATMENU:
			cAt2PMenu[1] = (Socket->readOct() == 1);
			if (cAt2PMenu[1])
				cout << ">> Player 2 is ready on the 2P menu!" << endl;
			else
				cout << ">> Player 2 is no longer on the 2P menu." << endl;
			return;

		case MSG_2PREADY:
			PlayerReady[1] = local.menu.PlayerReady[1] = Socket->readChar();

			cout << ">> Player 2 ready state changed." << endl;
			return;

		case MSG_M_CHARSEL:
			CharacterSelection[1] = local.menu.CharacterSelection[1] = Socket->readChar();

			return;

		case MSG_M_CHARCHOSEN:
			CharacterSelected[1] = local.menu.CharacterSelected[1] = Socket->readChar();
			return;

		case MSG_M_ALTCHAR:
			AltCharacterSonic = local.menu.AltCharacterSonic = Socket->readChar();
			AltCharacterShadow = local.menu.AltCharacterShadow = Socket->readChar();

			AltCharacterTails = local.menu.AltCharacterTails = Socket->readChar();
			AltCharacterEggman = local.menu.AltCharacterEggman = Socket->readChar();

			AltCharacterKnuckles = local.menu.AltCharacterKnuckles = Socket->readChar();
			AltCharacterRouge = local.menu.AltCharacterRouge = Socket->readChar();

			return;

		case MSG_S_BATTLEOPT:
			for (int i = 0; i < 4; i++)
				BattleOptions[i] = local.menu.BattleOptions[i] = Socket->readChar();

			return;

		case MSG_M_BATTLEOPTSEL:
			BattleOptionsSelection = local.menu.BattleOptionsSelection = Socket->readChar();
			BattleOptionsBackSelected = local.menu.BattleOptionsBackSelected = Socket->readChar();

			return;

		case MSG_M_STAGESEL:
			StageSelection2P[0] = local.menu.StageSelection2P[0] = Socket->readInt();
			StageSelection2P[1] = local.menu.StageSelection2P[1] = Socket->readInt();
			BattleOptionsButton = local.menu.BattleOptionsButton = Socket->readChar();

			return;

		case MSG_M_BATTLEMODESEL:
			BattleSelection = local.menu.BattleSelection = Socket->readChar();

			return;
		}
	}
	else
		return;
}

void MemoryHandler::PreReceive()
{
	updateAbstractPlayer(&recvPlayer, MainCharacter[1]);

	p2Input->read();

	writeRings();
	writeSpecials();

	return;
}

void MemoryHandler::PostReceive()
{
	updateAbstractPlayer(&recvPlayer, MainCharacter[1]);
	writeP2Memory();

	p2Input->read();

	writeRings();
	writeSpecials();

	return;
}