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

// The very manifestation of science

const float positionDelta = 16;
uint positionTimer = 0;
inline const bool PositionDelta(const Vertex& last, const Vertex& current)
{
	return (abs(last.x - current.x) >= positionDelta
		|| abs(last.y - current.y) >= positionDelta
		|| abs(last.z - current.z) >= positionDelta
		|| memcmp(&last, &current, sizeof(Vertex)) != 0 && Duration(positionTimer) >= 1250);
}

const uint rotateDelta = (uint)(5.625 * (65536 / 360));
uint rotateTimer = 0;
inline const bool RotationDelta(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateDelta
		|| abs(last.y - current.y) >= rotateDelta
		|| abs(last.z - current.z) >= rotateDelta
		|| memcmp(&last, &current, sizeof(Rotation)) != 0 && Duration(rotateTimer) >= 125);
}

const float speedDelta = 0.1F;
uint speedTimer = 0;
const bool SpeedDelta(const float last, const float current)
{
	return (
		//last != current // <- Old behavior for testing purposes.
		//abs(last - current) >= max((speedDelta * current), 0.01)
		abs(last - current) >= speedDelta
		//|| last != current && current <= speedDelta
		|| last != current && Duration(speedTimer) >= 125
		);
}


/*
//	Memory Handler Class
*/

// TODO: Consider using one large packet as opposed to several per-group packets.

MemoryHandler::MemoryHandler()
{
	Initialize();
}
void MemoryHandler::Initialize()
{
	local = {};
	recvInput = {};
	sendInput = {};

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
	sendSpinTimer = false;

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

	if (GameState < GameState::LOAD_FINISHED && (Controller1Raw.HeldButtons & Buttons_Y)
		|| GameState < GameState::LOAD_FINISHED && (recvInput.HeldButtons & Buttons_Y))
	{
		cout << "Warping to test level!" << endl;
		CurrentLevel = 0;
	}

	PacketEx safe(true), fast(false);

	SendInput(safe, fast);
	if (!CheckFrame())
	{
		SendSystem(safe, fast);
		SendPlayer(safe, fast);
		SendMenu(safe, fast);
	}

	Globals::Networking->Send(safe);
	Globals::Networking->Send(fast);

	// SetFrame() is called from outside this function.
}

void MemoryHandler::Receive(sf::Packet& packet, const bool safe)
{
	Socket::Status status = Socket::Status::NotReady;
	if (safe)
		status = Globals::Networking->recvSafe(packet);
	else
		status = Globals::Networking->recvFast(packet);

	// HACK: This isn't really a sufficient fix for the scale bug.
	// I suspect it's causing some weird side effects like "falling" while going down a slope,
	// usually interrupting spindashes. However, it fixes the scale issue.
	// (where the scale would be received, but overwritten with 0 before it could be applied to the player due to this function call)
	if (!writePlayer)
		UpdateAbstractPlayer(&recvPlayer, Player2);

	if (status == sf::Socket::Status::Done)
	{
		uchar id = MSG_NULL;
		uchar lastID = MSG_NULL;

		while (!packet.endOfPacket())
		{
			packet >> id;

			if (lastID == id)
			{
				cout << "\a<> Packet read loop failsafe! [LAST " << (ushort)lastID << " - RECV " << (ushort)id << ']' << endl;
				break;
			}

			//cout << (ushort)id << endl;
			switch (id)
			{
			case MSG_NULL:
				cout << "\a>> Reached end of packet." << endl;
				break;

			case MSG_COUNT:
				cout << ">> Received message count?! Malformed packet warning!" << endl;
				break;

			case MSG_DISCONNECT:
				cout << ">> Received disconnect request from client." << endl;
				Globals::Networking->Disconnect(true);
				break;

			default:
				ReceiveInput(id, packet);
				ReceiveSystem(id, packet);
				
				if (ReceivePlayer(id, packet))
					writeP2Memory();

				ReceiveMenu(id, packet);
				break;
			}

			lastID = id;
		}
	}
}

#pragma region Send

void MemoryHandler::SendSystem(PacketEx& safe, PacketEx& fast)
{
	if (GameState > GameState::LOAD_FINISHED && TwoPlayerMode > 0)
	{
		if (local.game.CurrentLevel != CurrentLevel)
		{
			RingCount[0] = 0;
			local.game.RingCount[0] = 0;

			sendSpinTimer = (Player1->Data2->CharID2 == Characters_Sonic
				|| Player1->Data2->CharID2 == Characters_Shadow
				|| Player1->Data2->CharID2 == Characters_Amy
				|| Player1->Data2->CharID2 == Characters_MetalSonic);

			local.game.CurrentLevel = CurrentLevel;
		}

		if (local.system.GameState != GameState)
			RequestPacket(MSG_S_GAMESTATE, safe, fast);

		if (GameState == GameState::PAUSE && local.system.PauseSelection != PauseSelection)
			RequestPacket(MSG_S_PAUSESEL, safe, fast);

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking->isServer())
			RequestPacket(MSG_S_TIME, fast, safe);

		if (local.game.TimeStopMode != TimeStopMode)
			RequestPacket(MSG_S_TIMESTOP, safe, fast);

		if (memcmp(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3) != 0)
			RequestPacket(MSG_S_2PSPECIALS, safe, fast);

		if (local.game.RingCount[0] != RingCount[0] && GameState == GameState::INGAME)
			RequestPacket(MSG_S_RINGS, safe, fast);
	}
}
void MemoryHandler::SendInput(PacketEx& safe, PacketEx& fast)
{
	if (CurrentMenu[0] == Menu::BATTLE || CurrentMenu[0] == Menu::BATTLE && TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		if (!CheckFrame())
			ToggleSplitscreen();

		if (sendInput.HeldButtons != ControllersRaw[0].HeldButtons)
		{
			// If the Action Button is pressed, then check the specials.
			if (ControllersRaw[0].PressedButtons & (Buttons_B | Buttons_X))
			{
				bool failsafe = false;

				for (uint8 i = 0; i < 3; i++)
				{
					// If a special has been used, send buttons followed immediately by specials.
					if (P1SpecialAttacks[i] == 0 && local.game.P1SpecialAttacks[i] != 0)
					{
						//cout << "<> \aUSED" << endl;
						// As it stands now, the special will rarely be 0 before we send it over, so if we set the local copy to 0,
						// it has a lower chance of sending redundant packets later, as SendSystem is behind a frame-sync barrier, whereas this is not.
						failsafe = true;
						RequestPacket(MSG_I_BUTTONS, safe, fast);
						RequestPacket(MSG_S_2PSPECIALS, safe, fast);
						local.game.P1SpecialAttacks[i] = 0;
						break;
					}
					// Otherwise, if a special has been gained, send specials first followed immediately by buttons.
					else if (P1SpecialAttacks[i] == 1 && local.game.P1SpecialAttacks[i] != 1)
					{
						//cout << "<> \aGAINED" << endl;
						failsafe = true;
						RequestPacket(MSG_S_2PSPECIALS, safe, fast);
						RequestPacket(MSG_I_BUTTONS, safe, fast);
						local.game.P1SpecialAttacks[i] = 1;
						break;
					}
				}

				if (!failsafe)
					RequestPacket(MSG_I_BUTTONS, safe, fast);
			}
			else
			{
				RequestPacket(MSG_I_BUTTONS, safe, fast);
			}
		}

		if (sendInput.LeftStickX != ControllersRaw[0].LeftStickX || sendInput.LeftStickY != ControllersRaw[0].LeftStickY)
		{
			if (GameState == GameState::INGAME /*&& Duration(analogTimer) >= 125*/)
			{
				if (ControllersRaw[0].LeftStickX == 0 && ControllersRaw[0].LeftStickY == 0)
					RequestPacket(MSG_I_ANALOG, safe, fast);
				else
					RequestPacket(MSG_I_ANALOG, fast, safe);

				analogTimer = millisecs();
			}
		}
	}
}
void MemoryHandler::SendPlayer(PacketEx& safe, PacketEx& fast)
{
	if (GameState >= GameState::LOAD_FINISHED && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (CheckTeleport())
		{
			RequestPacket(MSG_P_POSITION, safe, fast);
			RequestPacket(MSG_P_SPEED, safe, fast);
		}

		if (PositionDelta(sendPlayer.Data1.Position, Player1->Data1->Position))
			RequestPacket(MSG_P_POSITION, fast, safe);

		if (sendSpinTimer && sendPlayer.Sonic.SpindashTimer != ((SonicCharObj2*)Player1->Data2)->SpindashTimer)
			RequestPacket(MSG_P_SPINTIMER, safe, fast);

		if (sendPlayer.Data1.Action != Player1->Data1->Action || sendPlayer.Data1.Status != Player1->Data1->Status)
		{
			// IN CASE OF EMERGENCY, UNCOMMENT
			//cout << (ushort)sendPlayer.Data1.Action << " != " << (ushort)Player1->Data1->Action << " || " << sendPlayer.Data1.Status << " != " << Player1->Data1->Status << endl;

			if (sendPlayer.Data1.Action != 0x13 && Player1->Data1->Action == 0x15
				|| sendPlayer.Data1.Action == 0x20)
			{
				RequestPacket(MSG_P_ACTION, safe, fast);
				RequestPacket(MSG_P_STATUS, safe, fast);
			}

			RequestPacket(MSG_P_ANIMATION, safe, fast);
			RequestPacket(MSG_P_POSITION, safe, fast);

			if (sendSpinTimer)
				RequestPacket(MSG_P_SPINTIMER, safe, fast);
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

			// These are set in AddPacket
			//rotateTimer = millisecs();
			//speedTimer = rotateTimer;

			RequestPacket(MSG_P_ROTATION, fast, safe);
			RequestPacket(MSG_P_POSITION, fast, safe);
			RequestPacket(MSG_P_SPEED, fast, safe);
		}

		if (memcmp(&sendPlayer.Data1.Scale, &Player1->Data1->Scale, sizeof(Vertex)) != 0)
			RequestPacket(MSG_P_SCALE, safe, fast);

		if ((Player1->Data2->CharID == Characters_MechTails || Player1->Data2->CharID == Characters_MechEggman) && (sendPlayer.Data2.MechHP != Player1->Data2->MechHP))
			RequestPacket(MSG_P_HP, safe, fast);

		if (sendPlayer.Data2.Powerups != Player1->Data2->Powerups)
			RequestPacket(MSG_P_POWERUPS, safe, fast);

		if (sendPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
			RequestPacket(MSG_P_UPGRADES, safe, fast);

		UpdateAbstractPlayer(&sendPlayer, Player1);
	}
}
void MemoryHandler::SendMenu(PacketEx& safe, PacketEx& fast)
{
	if (GameState == GameState::INACTIVE && CurrentMenu[0] == Menu::BATTLE)
	{
		// Skip the Press Start screen straight to "Ready" screen
		if (CurrentMenu[1] >= SubMenu2P::S_START && P2Start != 2 && !wroteP2Start)
		{
			wroteP2Start = true;
			P2Start = 2;
		}

		// Send battle options
		if (memcmp(local.menu.BattleOptions, BattleOptions, BattleOptions_Length) != 0)
			RequestPacket(MSG_S_BATTLEOPT, safe);

		// Always send information about the menu you enter,
		// regardless of detected change.
		if ((firstMenuEntry = (local.menu.sub != CurrentMenu[1] && Globals::Networking->isServer())))
			local.menu.sub = CurrentMenu[1];

		switch (CurrentMenu[1])
		{
		default:
			break;

		case SubMenu2P::S_READY:
		case SubMenu2P::O_READY:
			if (firstMenuEntry || local.menu.PlayerReady[0] != PlayerReady[0])
				RequestPacket(MSG_S_2PREADY, safe);
			break;

		case SubMenu2P::S_BATTLEMODE:
			if (firstMenuEntry || local.menu.BattleSelection != BattleSelection)
				RequestPacket(MSG_M_BATTLESEL, safe);

			break;

		case SubMenu2P::S_CHARSEL:
		case SubMenu2P::O_CHARSEL:
			// HACK: Character select bug work-around. Details below.
			// When a button press is missed but the character selected state is synchronized,
			// the sub menu does not change to O_CHARSEL, so it won't progress. This forces it to.
			if (CharacterSelected[0] && CharacterSelected[1] && CurrentMenu[1] == SubMenu2P::S_CHARSEL)
			{
				cout << "<> Resetting character selections" << endl;
				CharacterSelectTimer = 0;
				CurrentMenu[1] = SubMenu2P::O_CHARSEL;
			}

			if (firstMenuEntry || local.menu.CharacterSelection[0] != CharacterSelection[0])
				RequestPacket(MSG_M_CHARSEL, safe);
			if (firstMenuEntry || local.menu.CharacterSelected[0] != CharacterSelected[0])
				RequestPacket(MSG_M_CHARCHOSEN, safe);

			// I hate this so much
			if (firstMenuEntry || (local.menu.AltCharacterSonic != AltCharacterSonic)
				|| (local.menu.AltCharacterShadow != AltCharacterShadow)
				|| (local.menu.AltCharacterTails != AltCharacterTails)
				|| (local.menu.AltCharacterEggman != AltCharacterEggman)
				|| (local.menu.AltCharacterKnuckles != AltCharacterKnuckles)
				|| (local.menu.AltCharacterRouge != AltCharacterRouge))
			{
				RequestPacket(MSG_M_ALTCHAR, safe);
			}

			break;

		case SubMenu2P::S_STAGESEL:
			if (firstMenuEntry
				|| local.menu.StageSelection2P[0] != StageSelection2P[0] || local.menu.StageSelection2P[1] != StageSelection2P[1]
				|| local.menu.BattleOptionsButton != BattleOptionsButton)
			{
				RequestPacket(MSG_M_STAGESEL, safe);
			}
			break;

		case SubMenu2P::S_BATTLEOPT:
			if (firstMenuEntry || local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBack != BattleOptionsBack)
				RequestPacket(MSG_M_BATTLEOPTSEL, safe);

			break;
		}
	}
}

const bool MemoryHandler::RequestPacket(const uchar packetType, PacketEx& packetAddTo, PacketEx& packetIsIn)
{
	if (packetType >= MSG_DISCONNECT && !packetIsIn.isInPacket(packetType) && packetAddTo.addType(packetType))
		return AddPacket(packetType, packetAddTo);

	return false;
}
const bool MemoryHandler::RequestPacket(const uchar packetType, PacketEx& packetAddTo)
{
	if (packetType >= MSG_DISCONNECT && packetAddTo.addType(packetType))
		return AddPacket(packetType, packetAddTo);

	return false;
}
const bool MemoryHandler::AddPacket(const uchar packetType, PacketEx& packet)
{
	switch (packetType)
	{
	default:
		return false;

#pragma region Input

	case MSG_I_ANALOG:
		packet << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
		sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
		sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
		return true;

	case MSG_I_BUTTONS:
		packet << ControllersRaw[0].HeldButtons;
		sendInput.HeldButtons = ControllersRaw[0].HeldButtons;
		return true;

#pragma endregion

#pragma region Menu

	case MSG_M_ALTCHAR:
		packet << AltCharacterSonic << AltCharacterShadow
			<< AltCharacterTails << AltCharacterEggman
			<< AltCharacterKnuckles << AltCharacterRouge;

		local.menu.AltCharacterSonic = AltCharacterSonic;
		local.menu.AltCharacterShadow = AltCharacterShadow;
		local.menu.AltCharacterTails = AltCharacterTails;
		local.menu.AltCharacterEggman = AltCharacterEggman;
		local.menu.AltCharacterKnuckles = AltCharacterKnuckles;
		local.menu.AltCharacterRouge = AltCharacterRouge;
		return true;

	case MSG_M_ATMENU:	// Deprecated
		return false;

	case MSG_M_BATTLESEL:
		packet << BattleSelection;
		local.menu.BattleSelection = BattleSelection;
		return true;

	case MSG_M_BATTLEOPTSEL:
		packet << BattleOptionsSelection << BattleOptionsBack;
		local.menu.BattleOptionsSelection = BattleOptionsSelection;
		local.menu.BattleOptionsBack = BattleOptionsBack;
		return true;

	case MSG_M_CHARCHOSEN:
		packet << CharacterSelected[0];
		local.menu.CharacterSelected[0] = CharacterSelected[0];
		return true;

	case MSG_M_CHARSEL:
		packet << CharacterSelection[0];
		local.menu.CharacterSelection[0] = CharacterSelection[0];
		return true;

	case MSG_M_STAGESEL:
		packet << StageSelection2P[0] << StageSelection2P[1] << BattleOptionsButton;
		local.menu.StageSelection2P[0] = StageSelection2P[0];
		local.menu.StageSelection2P[1] = StageSelection2P[1];
		local.menu.BattleOptionsButton = BattleOptionsButton;
		return true;

#pragma endregion

#pragma region Player

		// BEFORE YOU FORGET:
		// The reason sendPlayer is not updated here is because it's done in a separate function all at once.
		// Don't freak out!

	case MSG_P_ACTION:
		packet << Player1->Data1->Action;
		return true;

	case MSG_P_STATUS:
		packet << Player1->Data1->Status;
		return true;

	case MSG_P_ROTATION:
		speedTimer = rotateTimer = millisecs();
		packet << Player1->Data1->Rotation;
		return true;

	case MSG_P_POSITION:
		// Informs other conditions that it shouldn't request
		// another position packet so soon
		positionTimer = millisecs();
		packet << Player1->Data1->Position;
		return true;

	case MSG_P_SCALE:
		packet << Player1->Data1->Scale;
		return true;

	case MSG_P_CHARACTER:	// Not yet implemented.
		return false;

	case MSG_P_POWERUPS:
		cout << "<< Sending powerups" << endl;
		packet << Player1->Data2->Powerups;
		return true;

	case MSG_P_UPGRADES:
		cout << "<< Sending upgrades" << endl;
		packet << Player1->Data2->Upgrades;
		return true;

	case MSG_P_HP:
		packet << Player1->Data2->MechHP;
		return true;

	case MSG_P_SPEED:
		rotateTimer = speedTimer = millisecs();
		packet << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
		return true;

	case MSG_P_ANIMATION:
		packet << Player1->Data2->AnimInfo.Next;
		return true;

	case MSG_P_SPINTIMER:
		//cout << "<< [" << millisecs() << "]\t\tSPIN TIMER: " << ((SonicCharObj2*)Player1->Data2)->SpindashTimer << endl;
		packet << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
		return true;

#pragma endregion

#pragma region System

	case MSG_S_2PMODE:	// Not yet implemented.
		return false;

	case MSG_S_2PREADY:
		packet << PlayerReady[0];
		local.menu.PlayerReady[0] = PlayerReady[0];
		return true;

	case MSG_S_2PSPECIALS:
		packet.append(P1SpecialAttacks, sizeof(char) * 3);
		memcpy(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3);
		return true;

	case MSG_S_BATTLEOPT:
		packet.append(BattleOptions, BattleOptions_Length);
		memcpy(local.menu.BattleOptions, BattleOptions, BattleOptions_Length);
		return true;

	case MSG_S_GAMESTATE:
		packet << GameState;
		cout << "<< GameState [" << (ushort)local.system.GameState << ' ' << (ushort)GameState << ']' << endl;
		local.system.GameState = GameState;
		return true;

	case MSG_S_LEVEL:	// Not yet implemented
		return false;

	case MSG_S_PAUSESEL:
		packet << PauseSelection;
		local.system.PauseSelection = PauseSelection;
		return true;

	case MSG_S_RINGS:
		packet << RingCount[0];
		local.game.RingCount[0] = RingCount[0];
		return true;

	case MSG_S_TIME:
		packet << TimerMinutes << TimerSeconds << TimerFrames;
		memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
		return true;

	case MSG_S_TIMESTOP:
		cout << "<< Sending Time Stop [";

		// Swap the Time Stop value, as this is connected to player number,
		// and Player 1 and 2 are relative to the game instance.

		packet << (char)(TimeStopMode * 5 % 3);

		cout << ']' << endl;
		local.game.TimeStopMode = TimeStopMode;
		return true;


#pragma endregion

	}
}

#pragma endregion
#pragma region Receive

inline void MemoryHandler::writeP2Memory()
{
	if (GameState >= GameState::INGAME)
	{
		writePlayer = false;
		PlayerObject::WritePlayer(Player2, &recvPlayer);
	}
}

bool MemoryHandler::ReceiveInput(uchar type, sf::Packet& packet)
{
	if (CurrentMenu[0] == Menu::BATTLE || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_I_BUTTONS);
			packet >> recvInput.HeldButtons;
			if (CheckFrame())
				MemManage::waitFrame();
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

		case MSG_S_TIME:
			packet >> local.game.TimerMinutes >> local.game.TimerSeconds >> local.game.TimerFrames;
			TimerMinutes = local.game.TimerMinutes;
			TimerSeconds = local.game.TimerSeconds;
			TimerFrames = local.game.TimerFrames;
			return true;

			RECEIVED(MSG_S_GAMESTATE);
			{
				uchar recvGameState;
				packet >> recvGameState;
				if (GameState >= GameState::NORMAL_RESTART && recvGameState > GameState::LOAD_FINISHED)
					GameState = local.system.GameState = recvGameState;

				return true;
			}

			RECEIVED(MSG_S_PAUSESEL);
			packet >> local.system.PauseSelection;
			PauseSelection = local.system.PauseSelection;
			return true;

			RECEIVED(MSG_S_TIMESTOP);
			packet >> local.game.TimeStopMode;
			writeTimeStop();
			return true;

			RECEIVED(MSG_S_2PSPECIALS);
			for (int i = 0; i < 3; i++)
				packet >> local.game.P2SpecialAttacks[i];

			writeSpecials();
			return true;

			RECEIVED(MSG_S_RINGS);
			packet >> local.game.RingCount[1];
			writeRings();

			cout << ">> Ring Count Change " << local.game.RingCount[1] << endl;
			return true;
		}
	}
	return false;
}
bool MemoryHandler::ReceivePlayer(uchar type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		//writePlayer = false;
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_P_ACTION);
			packet >> recvPlayer.Data1.Action;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_STATUS);
			packet >> recvPlayer.Data1.Status;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_ROTATION);
			//case MSG_P_ROTATION:
			packet >> recvPlayer.Data1.Rotation;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_POSITION);
			//case MSG_P_POSITION:
			packet >> recvPlayer.Data1.Position;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SCALE);
			packet >> recvPlayer.Data1.Scale;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_POWERUPS);
			{
				int powerups;
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

			RECEIVED(MSG_P_HP);
			packet >> recvPlayer.Data2.MechHP;
			cout << ">> Received HP update. (" << recvPlayer.Data2.MechHP << ')' << endl;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SPEED);
			//case MSG_P_SPEED:
			packet >> recvPlayer.Data2.HSpeed;
			packet >> recvPlayer.Data2.VSpeed;
			packet >> recvPlayer.Data2.PhysData.BaseSpeed;

			writePlayer = true;
			break;

			RECEIVED(MSG_P_ANIMATION);
			packet >> recvPlayer.Data2.AnimInfo.Next;
			writePlayer = true;
			break;

			RECEIVED(MSG_P_SPINTIMER);
			//case MSG_P_SPINTIMER:
			packet >> recvPlayer.Sonic.SpindashTimer;
			//cout << ">> [" << millisecs() << "]\t\tSPIN TIMER: " << recvPlayer.Sonic.SpindashTimer << endl;
			writePlayer = true;
			break;
		}

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

			RECEIVED(MSG_M_ATMENU);	// Deprecated
			packet >> cAt2PMenu[1];
			if (cAt2PMenu[1])
				cout << ">> Player 2 is ready on the 2P menu!" << endl;
			else
				cout << ">> Player 2 is no longer on the 2P menu." << endl;
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
				>> local.menu.BattleOptionsBack;
			BattleOptionsSelection = local.menu.BattleOptionsSelection;
			BattleOptionsBack = local.menu.BattleOptionsBack;

			return true;

			RECEIVED(MSG_M_STAGESEL);
			packet >> local.menu.StageSelection2P[0]
				>> local.menu.StageSelection2P[1]
				>> local.menu.BattleOptionsButton;
			StageSelection2P[0] = local.menu.StageSelection2P[0];
			StageSelection2P[1] = local.menu.StageSelection2P[1];
			BattleOptionsButton = local.menu.BattleOptionsButton;

			return true;

			RECEIVED(MSG_M_BATTLESEL);
			packet >> local.menu.BattleSelection;
			BattleSelection = local.menu.BattleSelection;

			return true;
		}
	}
	return false;
}

void MemoryHandler::PreReceive()
{
	//UpdateAbstractPlayer(&recvPlayer, Player2);

	writeRings();
	writeSpecials();

	if (GameState == GameState::PAUSE &&
		(recvInput.LeftStickX != 0 || recvInput.LeftStickY != 0 || ControllersRaw[1].LeftStickX != 0 || ControllersRaw[1].LeftStickY != 0))
	{
		recvInput.LeftStickX = 0;
		recvInput.LeftStickY = 0;
		ControllersRaw[1].LeftStickX = 0;
		ControllersRaw[1].LeftStickY = 0;
	}
}
void MemoryHandler::PostReceive()
{
	//UpdateAbstractPlayer(&recvPlayer, Player2);
	writeP2Memory();

	writeRings();
	writeSpecials();
}

#pragma endregion
#pragma region Crap

inline void MemoryHandler::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

void MemoryHandler::UpdateAbstractPlayer(PlayerObject* destination, ObjectMaster* source)
{
	// HACK: Mech HP synchronization fix. This REALLY sucks.
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
		if ((sendInput.HeldButtons & Buttons_L) && (sendInput.HeldButtons & Buttons_R))
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

const unsigned int MemoryHandler::GetCurrentMenu()
{
	if (!Globals::Networking->isConnected())
		GetFrame();

	if (CheckFrame())
		return local.menu.main;
	else
		return local.menu.main = CurrentMenu[0];
}