// Defines

#define RECEIVED RECEIVE_VERBOSE
#define RECEIVE_VERBOSE(type) case type: cout << ">> Received type " #type << endl
#define RECEIVE_CONCISE(type) case type:;

// Makes sure TYPE isn't in ISIN and adds it to ADDTO
#define CheckAndAdd(TYPE, ISIN, ADDTO) !ISIN.isInPacket(TYPE) && ADDTO.addType(TYPE)

#define WIN32_LEAN_AND_MEAN

// Standard Includes
#include <iostream>
#include <Windows.h>
#include <cmath>	// for abs

// Global Includes
#include <LazyTypedefs.h>

// Local Includes
#include "Globals.h"
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketExtensions.h"
#include "AdventurePacketOverloads.h"
#include "PacketHandler.h"

#include "SA2ModLoader.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"
#include "ActionBlacklist.h"

// This Class
#include "MemoryHandler.h"

// Namespaces
using namespace std;
using namespace sf;
using namespace sa2bn;

const float positionDelta = 16;
uint positionTimer = 0;
inline const bool PositionDelta(const Vertex& last, const Vertex& current)
{
	return (abs(last.x - current.x) >= positionDelta
		|| abs(last.y - current.y) >= positionDelta
		|| abs(last.z - current.z) >= positionDelta
		|| memcmp(&last, &current, sizeof(Vertex)) != 0 && Duration(positionTimer) >= 1250);
}

const uint rotateDelta = ((float)11.25 * (float)(65536 / 360));
uint rotateTimer = 0;
inline const bool RotationDelta(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateDelta
		|| abs(last.y - current.y) >= rotateDelta
		|| abs(last.z - current.z) >= rotateDelta
		|| memcmp(&last, &current, sizeof(Rotation)) != 0 && Duration(rotateTimer) >= 1250);
}

const float speedDelta = 0.125;
uint speedTimer = 0;
const bool SpeedDelta(const float last, const float current)
{
	return (abs(last - current) >= max((speedDelta * current), 0.01)
		//|| last != current && current < speedDelta
		|| last != current && Duration(speedTimer) >= 1250);
}


/*
//	Memory Handler Class
*/

MemoryHandler::MemoryHandler() : local(), recvInput(), sendInput()
{
	//memset(this, 0, sizeof(MemoryHandler));

	analogTimer = 0;

	cAt2PMenu[0] = false;
	cAt2PMenu[1] = false;
	lAt2PMenu[0] = false;
	lAt2PMenu[1] = false;

	firstMenuEntry = false;
	wroteP2Start = false;
	splitToggled = false;
	Teleported = false;
	writePlayer = false;

	thisFrame = 0;
	lastFrame = 0;
}


void MemoryHandler::RecvLoop()
{
	if (Globals::Networking->isConnected())
	{
		// Grab the current frame before continuing.
		// This is for frame synchronization.
		GetFrame();
		PreReceive();

		//if (CurrentMenu[0] < Menu::BATTLE)
		//	Globals::Networking->Disconnect();

		sf::Packet packet;
		Receive(packet, true);
		Receive(packet, false);

		PostReceive();
		// SetFrame() is called from outside this function.
	}
}
void MemoryHandler::SendLoop()
{
	// Grab the current frame before continuing.
	// This is for frame synchronization.
	GetFrame();

	SendInput();
	if (!CheckFrame())
	{
		SendSystem();
		SendPlayer();
		SendMenu();
	}

	// SetFrame() is called from outside this function.
}

void MemoryHandler::Receive(sf::Packet& packet, const bool safe)
{
	Socket::Status status = Socket::Status::NotReady;
	if (safe)
		status = Globals::Networking->recvSafe(packet);
	else
		status = Globals::Networking->recvFast(packet);

	if (status == sf::Socket::Status::Done)
	{
		uchar id = MSG_NULL;
		while (!packet.endOfPacket())
		{
			packet >> id;
			//cout << (ushort)id << endl;
			switch (id)
			{
				//case MSG_NULL:
				//	cout << "\a>> Reached end of packet." << endl;
				//	break;

			case MSG_COUNT:
				cout << ">> Received message count?! Malformed packet warning!" << endl;
				break;

			case MSG_DISCONNECT:
				cout << ">> Received disconnect request from client." << endl;
				Globals::Networking->Disconnect(true);
				break;

			default:
				ReceiveSystem(id, packet);
				ReceiveInput(id, packet);
				ReceivePlayer(id, packet);
				ReceiveMenu(id, packet);
				break;
			}
		}
	}
}

#pragma region Send

void MemoryHandler::SendSystem()
{
	if (GameState >= GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		PacketEx safe(true), fast(false);

		if (local.system.GameState != GameState)
		{
			if (CheckAndAdd(MSG_S_GAMESTATE, fast, safe))
			{
				safe << GameState;
				cout << "<< GameState [" << (ushort)local.system.GameState << ' ' << (ushort)GameState << ']' << endl;
				local.system.GameState = GameState;
			}
		}

		if (GameState == GameState::PAUSE && local.system.PauseSelection != PauseSelection)
		{
			if (CheckAndAdd(MSG_S_PAUSESEL, fast, safe))
			{
				safe << PauseSelection;
				local.system.PauseSelection = PauseSelection;
			}
		}

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking->isServer())
		{
			if (CheckAndAdd(MSG_S_TIME, safe, fast))
			{
				fast << TimerMinutes << TimerSeconds << TimerFrames;
				memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			}
		}

		if (local.game.TimeStopMode != TimeStopMode)
		{
			if (CheckAndAdd(MSG_S_TIMESTOP, fast, safe))
			{
				cout << "<< Sending Time Stop [";

				// Swap the Time Stop value, as this is connected to player number,
				// and Player 1 and 2 are relative to the game instance.

				safe << (char)(TimeStopMode * 5 % 3);

				cout << ']' << endl;
				local.game.TimeStopMode = TimeStopMode;
			}
		}

		if (memcmp(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3) != 0)
		{
			if (CheckAndAdd(MSG_S_2PSPECIALS, fast, safe))
			{
				safe.append(P1SpecialAttacks, sizeof(char) * 3);
				memcpy(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3);
			}
		}

		if (local.game.RingCount[0] != RingCount[0])
		{
			if (CheckAndAdd(MSG_S_RINGS, fast, safe))
			{
				safe << RingCount[0];
				local.game.RingCount[0] = RingCount[0];
			}
		}

		Globals::Networking->Send(fast);
		Globals::Networking->Send(safe);
	}
}
void MemoryHandler::SendInput(/*uint sendTimer*/)
{
	PacketEx safe(true), fast(false);

	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		if (!CheckFrame())
			ToggleSplitscreen();

		if (sendInput.HeldButtons != ControllersRaw[0].HeldButtons)
		{
			if (CheckAndAdd(MSG_I_BUTTONS, fast, safe))
			{
				safe << ControllersRaw[0].HeldButtons;
				sendInput.HeldButtons = ControllersRaw[0].HeldButtons;
			}
		}

		if (sendInput.LeftStickX != ControllersRaw[0].LeftStickX || sendInput.LeftStickY != ControllersRaw[0].LeftStickY)
		{
			if (GameState == GameState::INGAME)
			{
				if (Duration(analogTimer) >= 125)
				{
					if (ControllersRaw[0].LeftStickX == 0 && ControllersRaw[0].LeftStickY == 0)
					{
						if (CheckAndAdd(MSG_I_ANALOG, fast, safe))
						{
							safe << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
							sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
							sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
						}
					}
					else
					{
						if (CheckAndAdd(MSG_I_ANALOG, safe, fast))
						{
							fast << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
							sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
							sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
						}
					}

					analogTimer = millisecs();
				}
			}
			else if (sendInput.LeftStickY != 0 || sendInput.LeftStickX != 0)
			{
				if (CheckAndAdd(MSG_I_ANALOG, fast, safe))
				{
					cout << "<< Resetting analog" << endl;
					sendInput.LeftStickY = 0;
					sendInput.LeftStickX = 0;
					safe << sendInput.LeftStickY << sendInput.LeftStickX;
				}
			}
		}
	}

	Globals::Networking->Send(fast);
	Globals::Networking->Send(safe);
}
void MemoryHandler::SendPlayer()
{
	if (GameState >= GameState::LOAD_FINISHED && CurrentMenu[0] >= Menu::BATTLE)
	{
		PacketEx safe(true), fast(false);

		if (CheckTeleport())
		{
			if (CheckAndAdd(MSG_P_POSITION, fast, safe))
			{
				positionTimer = millisecs();
				safe << Player1->Data1->Position;
			}
			if (CheckAndAdd(MSG_P_SPEED, fast, safe))
				safe << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
		}

		if (PositionDelta(sendPlayer.Data1.Position, Player1->Data1->Position))
		{
			if (CheckAndAdd(MSG_P_POSITION, safe, fast))
			{
				positionTimer = millisecs();
				fast << Player1->Data1->Position;
			}
		}

		if (sendPlayer.Data1.Action != Player1->Data1->Action || sendPlayer.Data1.Status != Player1->Data1->Status)
		{
			bool sendSpinTimer = (Player1->Data2->CharID2 == Characters_Sonic
				|| Player1->Data2->CharID2 == Characters_Shadow
				|| Player1->Data2->CharID2 == Characters_Amy
				|| Player1->Data2->CharID2 == Characters_MetalSonic);

			// IN CASE OF EMERGENCY, UNCOMMENT
			//cout << (ushort)sendPlayer.Data1.Action << " != " << (ushort)Player1->Data1->Action << " || " << sendPlayer.Data1.Status << " != " << Player1->Data1->Status << endl;

			if (CheckAndAdd(MSG_P_ACTION, fast, safe))
				safe << Player1->Data1->Action;
			if (CheckAndAdd(MSG_P_STATUS, fast, safe))
				safe << Player1->Data1->Status;
			if (CheckAndAdd(MSG_P_POSITION, fast, safe))
			{
				positionTimer = millisecs();
				safe << Player1->Data1->Position;
			}
			if (sendSpinTimer && CheckAndAdd(MSG_P_SPINTIMER, fast, safe))
				safe << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
		}

		if (RotationDelta(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			|| (SpeedDelta(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) || SpeedDelta(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed))
			|| sendPlayer.Data2.PhysData.BaseSpeed != Player1->Data2->PhysData.BaseSpeed)
		{
			// IN CASE OF EMERGENCY, UNCOMMENT
			//cout << RotationDelta(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			//	<< " || (" << SpeedDelta(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) << " || " << SpeedDelta(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed)
			//	<< ") || " << sendPlayer.Data2.PhysData.BaseSpeed << " != " << Player1->Data2->PhysData.BaseSpeed
			//	<< endl;
			rotateTimer = millisecs();
			speedTimer = rotateTimer;
			if (CheckAndAdd(MSG_P_ROTATION, safe, fast))
				fast << Player1->Data1->Rotation;
			if (CheckAndAdd(MSG_P_POSITION, safe, fast))
			{
				positionTimer = millisecs();
				fast << Player1->Data1->Position;
			}
			if (CheckAndAdd(MSG_P_SPEED, safe, fast))
				fast << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
		}

		if (memcmp(&sendPlayer.Data1.Scale, &Player1->Data1->Scale, sizeof(Vertex)) != 0)
		{
			if (CheckAndAdd(MSG_P_SCALE, fast, safe))
				safe << Player1->Data1->Scale;
		}

		if ((Player1->Data2->CharID == 6 || Player1->Data2->CharID == 7) && (sendPlayer.Data2.MechHP != Player1->Data2->MechHP))
		{
			if (CheckAndAdd(MSG_P_HP, fast, safe))
				safe << Player1->Data2->MechHP;
		}

		if (sendPlayer.Data2.Powerups != Player1->Data2->Powerups)
		{
			if (CheckAndAdd(MSG_P_POWERUPS, fast, safe))
			{
				cout << "<< Sending powerups" << endl;
				safe << Player1->Data2->Powerups;
			}
		}
		if (sendPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
		{
			if (CheckAndAdd(MSG_P_UPGRADES, fast, safe))
			{
				cout << "<< Sending upgrades" << endl;
				safe << Player1->Data2->Upgrades;
			}
		}

		updateAbstractPlayer(&sendPlayer, Player1);

		Globals::Networking->Send(fast);
		Globals::Networking->Send(safe);
	}
}
void MemoryHandler::SendMenu()
{
	//PacketEx safe(true), fast(false);

	//Globals::Networking->Send(fast);
	//Globals::Networking->Send(safe);
}

#pragma endregion
#pragma region Receive

inline void MemoryHandler::writeP2Memory()
{
	if (GameState >= GameState::INGAME)
		PlayerObject::WritePlayer(Player2, &recvPlayer);
}

bool MemoryHandler::ReceiveInput(uchar type, sf::Packet& packet)
{
	if (CurrentMenu[0] == 16 || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_I_BUTTONS);
			packet >> recvInput.HeldButtons;
			if (CheckFrame())
				MemManage::waitFrame(1, thisFrame);
			recvInput.WriteButtons(ControllersRaw[1]);

			return true;

			RECEIVED(MSG_I_ANALOG);
			packet >> ControllersRaw[1].LeftStickX >> ControllersRaw[1].LeftStickY;
			return true;
		}
	}

	return false;
}
bool MemoryHandler::ReceiveSystem(uchar type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_S_TIME);

			packet >> local.game.TimerMinutes >> local.game.TimerSeconds >> local.game.TimerFrames;
			TimerMinutes = local.game.TimerMinutes;
			TimerSeconds = local.game.TimerSeconds;
			TimerFrames = local.game.TimerFrames;
			return true;

			RECEIVED(MSG_S_GAMESTATE);
			{

				uchar recvGameState;
				packet >> recvGameState;
				cout << (ushort)recvGameState << ' ' << (ushort)local.system.GameState << ' ' << (ushort)GameState << endl;
				if (GameState >= GameState::INGAME && recvGameState > GameState::LOAD_FINISHED)
					GameState = local.system.GameState = recvGameState;

				return true;
			}

			RECEIVED(MSG_S_PAUSESEL);
			packet >> local.system.PauseSelection;
			PauseSelection = local.system.PauseSelection;
			return true;

			RECEIVED(MSG_S_TIMESTOP);
			{

				uchar temp;
				packet >> temp;
				local.game.TimeStopMode = temp;
				writeTimeStop();
				return true;
			}

			RECEIVED(MSG_S_2PSPECIALS);
			{

				uchar specials[3];
				for (int i = 0; i < 3; i++)
					packet >> specials[i];

				memcpy(local.game.P2SpecialAttacks, specials, sizeof(char) * 3);

				writeSpecials();
				return true;
			}

			RECEIVED(MSG_S_RINGS);
			packet >> local.game.RingCount[1];
			writeRings();

			cout << ">> Ring Count Change); " << local.game.RingCount[1] << endl;
			return true;
		}
	}
	return false;
}
bool MemoryHandler::ReceivePlayer(uchar type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = false;
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_P_HP);
			packet >> recvPlayer.Data2.MechHP;
			cout << ">> Received HP update. (" << recvPlayer.Data2.MechHP << ')' << endl;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_ACTION);
			{
				uchar action;
				packet >> action;
				recvPlayer.Data1.Action = action;
				writePlayer = true;
				break;
			}

			RECEIVED(MSG_P_STATUS);
			packet >> recvPlayer.Data1.Status;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SPINTIMER);
			packet >> recvPlayer.Sonic.SpindashTimer;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_ANIMATION);
			packet >> recvPlayer.Data2.AnimInfo.Next;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_POSITION);
			packet >> recvPlayer.Data1.Position;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_ROTATION);
			packet >> recvPlayer.Data1.Rotation;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SCALE);
			packet >> recvPlayer.Data1.Scale;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SPEED);
			packet >> recvPlayer.Data2.HSpeed;
			packet >> recvPlayer.Data2.VSpeed;
			packet >> recvPlayer.Data2.PhysData.BaseSpeed;

			writePlayer = true;
			break;

			RECEIVED(MSG_P_POWERUPS);
			{
				short powerups;
				packet >> powerups;
				recvPlayer.Data2.Powerups = (Powerups)powerups;
				writePlayer = true;
				break;
			}

			RECEIVED(MSG_P_UPGRADES);
			{
				int upgrades;
				packet >> upgrades;
				recvPlayer.Data2.Upgrades = (Upgrades)upgrades;
				writePlayer = true;
				break;
			}
		}

		if (writePlayer)
			writeP2Memory();

		return writePlayer;
	}

	return false;
}
bool MemoryHandler::ReceiveMenu(uchar type, sf::Packet& packet)
{
	if (GameState == GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_M_ATMENU);
			packet >> cAt2PMenu[1];
			if (cAt2PMenu[1])
				cout << ">> Player 2 is ready on the 2P menu! ";
			else
				cout << ">> Player 2 is no longer on the 2P menu. ";
			cout << cAt2PMenu[1] << endl;
			return true;

			RECEIVED(MSG_S_2PREADY);
			packet >> local.menu.PlayerReady[1];
			PlayerReady[1] = local.menu.PlayerReady[1];

			cout << ">> Player 2 ready state changed. " << (ushort)local.menu.PlayerReady[1] << endl;
			return true;

			RECEIVED(MSG_M_CHARSEL);
			packet >> local.menu.CharacterSelection[1];
			CharacterSelection[1] = local.menu.CharacterSelection[1];
			cout << (ushort)local.menu.CharacterSelection[1] << ' ' << (ushort)CharacterSelection[1] << endl;
			return true;

			RECEIVED(MSG_M_CHARCHOSEN);
			packet >> local.menu.CharacterSelected[1];
			CharacterSelected[1] = local.menu.CharacterSelected[1];
			cout << (ushort)CharacterSelected[1] << ' ' << (ushort)local.menu.CharacterSelected[1] << endl;
			return true;

			RECEIVED(MSG_M_ALTCHAR);
			packet >> local.menu.AltCharacterSonic
				>> local.menu.AltCharacterShadow
				>> local.menu.AltCharacterTails
				>> local.menu.AltCharacterEggman
				>> local.menu.AltCharacterKnuckles
				>> local.menu.AltCharacterRouge;
			AltCharacterSonic = local.menu.AltCharacterSonic;
			AltCharacterShadow = local.menu.AltCharacterShadow;

			AltCharacterTails = local.menu.AltCharacterTails;
			AltCharacterEggman = local.menu.AltCharacterEggman;

			AltCharacterKnuckles = local.menu.AltCharacterKnuckles;
			AltCharacterRouge = local.menu.AltCharacterRouge;

			return true;

			RECEIVED(MSG_S_BATTLEOPT);
			for (int i = 0; i < 4; i++)
				packet >> local.menu.BattleOptions[i];
			memcpy(BattleOptions, local.menu.BattleOptions, sizeof(char) * 4);

			return true;

			RECEIVED(MSG_M_BATTLEOPTSEL);
			packet >> local.menu.BattleOptionsSelection
				>> local.menu.BattleOptionsBackSelected;
			BattleOptionsSelection = local.menu.BattleOptionsSelection;
			BattleOptionsBackSelected = local.menu.BattleOptionsBackSelected;

			return true;

			RECEIVED(MSG_M_STAGESEL);
			packet >> local.menu.StageSelection2P[0]
				>> local.menu.StageSelection2P[1]
				>> local.menu.BattleOptionsButton;
			StageSelection2P[0] = local.menu.StageSelection2P[0];
			StageSelection2P[1] = local.menu.StageSelection2P[1];
			BattleOptionsButton = local.menu.BattleOptionsButton;

			return true;

			RECEIVED(MSG_M_BATTLEMODESEL);
			packet >> local.menu.BattleSelection;
			BattleSelection = local.menu.BattleSelection;

			return true;
		}
	}
	return false;
}

void MemoryHandler::PreReceive()
{
	updateAbstractPlayer(&recvPlayer, Player2);

	writeRings();
	writeSpecials();

	return;
}
void MemoryHandler::PostReceive()
{
	updateAbstractPlayer(&recvPlayer, Player2);
	//writeP2Memory();

	writeRings();
	writeSpecials();

	return;
}

#pragma endregion
#pragma region Crap

inline void MemoryHandler::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

void MemoryHandler::updateAbstractPlayer(PlayerObject* destination, ObjectMaster* source)
{
	// Mech synchronize hack
	if (GameState >= GameState::INGAME && Player2 != nullptr)
	{
		if (Player2->Data2->CharID2 == Characters_MechEggman || Player2->Data2->CharID2 == Characters_MechTails)
			Player2->Data2->MechHP = recvPlayer.Data2.MechHP;
		Player2->Data2->Powerups = recvPlayer.Data2.Powerups;
		Player2->Data2->Upgrades = recvPlayer.Data2.Upgrades;
	}

	destination->Set(source);
}

#pragma endregion
#pragma region Toggles

void MemoryHandler::ToggleSplitscreen()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((sendInput.HeldButtons & (1 << 16)) && (sendInput.HeldButtons & (2 << 16)))
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
		if ((sendInput.HeldButtons & Buttons_Y) && (sendInput.HeldButtons & Buttons_Up))
		{
			if (!Teleported)
			{
				// Teleport to recvPlayer
				cout << "<> Teleporting to other player..." << endl;;
				PlayerObject::Teleport(&recvPlayer, Player1);

				return Teleported = true;
			}
		}
		else if (Teleported)
			return Teleported = false;
	}

	return false;
}

#pragma endregion