// Standard Includes

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

#include <SA2ModLoader.h>
#include "BAMS.h"
#include "MemoryManagement.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"

// This Class
#include "MemoryHandler.h"

// Namespaces
using namespace std;
using namespace sf;
using namespace sa2bn;

#pragma region science

// TODO: Re-evaluate all this science.
// TODO: Consider using the same timer for all three.

const float positionDelta = 16;
const int rotateDelta = toBAMS(5.625); // TODO: Consider adjusting this yet again. 11.25?
const float speedDelta = 0.1F;

uint positionTimer = 0;
uint rotateTimer = 0;
uint speedTimer = 0;

static inline bool PositionDelta(const Vertex& last, const Vertex& current)
{
	return (abs(last.x - current.x) >= positionDelta
		|| abs(last.y - current.y) >= positionDelta
		|| abs(last.z - current.z) >= positionDelta
		|| /*memcmp(&last, &current, sizeof(Vertex)) != 0 &&*/ Duration(positionTimer) >= 10000);
}

static inline bool RotationDelta(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateDelta
		|| abs(last.y - current.y) >= rotateDelta
		|| abs(last.z - current.z) >= rotateDelta
		|| Duration(rotateTimer) >= 125 && memcmp(&last, &current, sizeof(Rotation)) != 0);
}

static inline bool SpeedDelta(const float last, const float current)
{
	return last != current && (Duration(speedTimer) >= 10000 || abs(last - current) >= speedDelta);
	//abs(last - current) >= max((speedDelta * current), 0.01)
}

#pragma endregion

/*
//	Memory Handler Class
*/

// TODO: After rewriting the input handler yet again, consider frame sync blocking RecvLoop

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

	firstMenuEntry = false;
	wroteP2Start = false;
	writePlayer = false;
	sendSpinTimer = false;

	thisFrame = 0;
	lastFrame = 0;
}

void MemoryHandler::RecvLoop()
{
	// Grab the current frame before continuing.
	// This is for frame synchronization.
	//GetFrame();
	PreReceive();

	sf::Packet packet;
	Receive(packet, true);
	Receive(packet, false);

	PostReceive();

	// TODO: Consider moving the main logic loop to this class. See comment below.
	// HACK: SetFrame() is called from outside this function.
}
void MemoryHandler::SendLoop()
{
	// Grab the current frame before continuing.
	// This is for frame synchronization.
	GetFrame();

	if (isNewFrame() && (GameState < GameState::LOAD_FINISHED && (Controller1Raw.HeldButtons & Buttons_Y)
		|| GameState < GameState::LOAD_FINISHED && (recvInput.HeldButtons & Buttons_Y)))
	{
		PrintDebug("<> Warping to test level!");
		CurrentLevel = 0;
	}

	PacketEx safe(true), fast(false);

	SendInput(safe, fast);
	if (isNewFrame())
	{
		SendSystem(safe, fast);
		SendPlayer(safe, fast);
		SendMenu(safe, fast);
	}

	Globals::Networking->Send(safe);
	Globals::Networking->Send(fast);

	// HACK: SetFrame() is called from outside this function.
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
		uint8 newType = MSG_NULL;
		uint8 lastType = MSG_NULL;

		while (!packet.endOfPacket())
		{
			packet >> newType;

			if (newType == lastType)
			{
				PrintDebug("\a<> Packet read loop failsafe! [LAST %d - RECV %d]", lastType, newType);
				break;
			}

			//cout << (ushort)newType << endl;
			switch (newType)
			{
			case MSG_NULL:
				PrintDebug("\a>> Reached end of packet.");
				break;

			case MSG_COUNT:
				PrintDebug(">> Received message count?! Malformed packet warning!");
				break;

			case MSG_DISCONNECT:
				PrintDebug(">> Received disconnect request from client.");
				Globals::Networking->Disconnect(true);
				break;

			default:

				ReceiveInput(newType, packet);
				ReceiveSystem(newType, packet);

				// HACK: This isn't really a sufficient fix for the scale bug.
				// I suspect it's causing some weird side effects like "falling" while going down a slope,
				// usually interrupting spindashes. However, it fixes the scale issue.
				// (where the scale would be received, but overwritten with 0 before it could be applied to the player due to this function call)
				if (!writePlayer)
					recvPlayer.Set(Player2);

				if (ReceivePlayer(newType, packet))
				{
					if (GameState >= GameState::INGAME)
					{
						writePlayer = false;
						PlayerObject::WritePlayer(Player2, &recvPlayer);
					}
				}

				ReceiveMenu(newType, packet);
				break;
			}

			lastType = newType;
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
		if (isNewFrame())
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
						//PrintDebug("<> \aUSED");
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
						//PrintDebug("<> \aGAINED");
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

				analogTimer = Millisecs();
			}
		}
	}
}
void MemoryHandler::SendPlayer(PacketEx& safe, PacketEx& fast)
{
	if (GameState >= GameState::LOAD_FINISHED && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (Teleport())
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
			// This "pickup crash fix" only fixed it by NEVER SENDING THE ACTION! Good job, me!
			// sendPlayer.Data1.Action != 0x13 && Player1->Data1->Action == 0x15 || sendPlayer.Data1.Action == 0x20
			RequestPacket(MSG_P_ACTION, safe, fast);
			RequestPacket(MSG_P_STATUS, safe, fast);

			RequestPacket(MSG_P_ANIMATION, safe, fast);
			RequestPacket(MSG_P_POSITION, safe, fast);

			if (sendSpinTimer)
				RequestPacket(MSG_P_SPINTIMER, safe, fast);
		}

		if (RotationDelta(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			|| (SpeedDelta(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) || SpeedDelta(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed))
			|| sendPlayer.Data2.PhysData.BaseSpeed != Player1->Data2->PhysData.BaseSpeed)
		{
			RequestPacket(MSG_P_ROTATION, fast, safe);
			RequestPacket(MSG_P_POSITION, fast, safe);
			RequestPacket(MSG_P_SPEED, fast, safe);
		}

		if (memcmp(&sendPlayer.Data1.Scale, &Player1->Data1->Scale, sizeof(Vertex)) != 0)
			RequestPacket(MSG_P_SCALE, safe, fast);

		if (Player1->Data2->CharID == Characters_MechTails || Player1->Data2->CharID == Characters_MechEggman)
		{
			if (sendPlayer.Data2.MechHP != Player1->Data2->MechHP)
				RequestPacket(MSG_P_HP, safe, fast);
		}

		if (sendPlayer.Data2.Powerups != Player1->Data2->Powerups)
			RequestPacket(MSG_P_POWERUPS, safe, fast);

		if (sendPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
			RequestPacket(MSG_P_UPGRADES, safe, fast);

		sendPlayer.Set(Player1);
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
				PrintDebug("<> Resetting character selections");
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

bool MemoryHandler::RequestPacket(const uint8 packetType, PacketEx& packetAddTo, PacketEx& packetIsIn)
{
	if (!packetIsIn.isInPacket(packetType))
		return RequestPacket(packetType, packetAddTo);

	return false;
}
bool MemoryHandler::RequestPacket(const uint8 packetType, PacketEx& packetAddTo)
{
	if (packetType >= MSG_DISCONNECT && packetAddTo.addType(packetType))
		return AddPacket(packetType, packetAddTo);

	return false;
}
bool MemoryHandler::AddPacket(const uint8 packetType, PacketEx& packet)
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
		break;

	case MSG_I_BUTTONS:
		packet << ControllersRaw[0].HeldButtons;
		sendInput.HeldButtons = ControllersRaw[0].HeldButtons;
		break;

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
		break;

	case MSG_M_BATTLESEL:
		packet << BattleSelection;
		local.menu.BattleSelection = BattleSelection;
		break;

	case MSG_M_BATTLEOPTSEL:
		packet << BattleOptionsSelection << BattleOptionsBack;
		local.menu.BattleOptionsSelection = BattleOptionsSelection;
		local.menu.BattleOptionsBack = BattleOptionsBack;
		break;

	case MSG_M_CHARCHOSEN:
		packet << CharacterSelected[0];
		local.menu.CharacterSelected[0] = CharacterSelected[0];
		break;

	case MSG_M_CHARSEL:
		packet << CharacterSelection[0];
		local.menu.CharacterSelection[0] = CharacterSelection[0];
		break;

	case MSG_M_STAGESEL:
		packet << StageSelection2P[0] << StageSelection2P[1] << BattleOptionsButton;
		local.menu.StageSelection2P[0] = StageSelection2P[0];
		local.menu.StageSelection2P[1] = StageSelection2P[1];
		local.menu.BattleOptionsButton = BattleOptionsButton;
		break;

#pragma endregion

#pragma region Player

		// BEFORE YOU FORGET:
		// The reason sendPlayer is not updated here is because it's done in a separate function all at once.
		// Don't freak out!

	case MSG_P_ACTION:
		packet << Player1->Data1->Action;
		break;

	case MSG_P_STATUS:
		packet << Player1->Data1->Status;
		break;

	case MSG_P_ROTATION:
		speedTimer = rotateTimer = Millisecs();
		packet << Player1->Data1->Rotation;
		break;

	case MSG_P_POSITION:
		// Informs other conditions that it shouldn't request
		// another position packet so soon
		positionTimer = Millisecs();
		packet << Player1->Data1->Position;
		break;

	case MSG_P_SCALE:
		packet << Player1->Data1->Scale;
		break;

	case MSG_P_CHARACTER:	// Not yet implemented.
		return false;

	case MSG_P_POWERUPS:
		PrintDebug("<< Sending powerups");
		packet << Player1->Data2->Powerups;
		break;

	case MSG_P_UPGRADES:
		PrintDebug("<< Sending upgrades");
		packet << Player1->Data2->Upgrades;
		break;

	case MSG_P_HP:
		packet << Player1->Data2->MechHP;
		break;

	case MSG_P_SPEED:
		rotateTimer = speedTimer = Millisecs();
		packet << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
		break;

	case MSG_P_ANIMATION:
		packet << Player1->Data2->AnimInfo.Next;
		break;

	case MSG_P_SPINTIMER:
		//cout << "<< [" << Millisecs() << "]\t\tSPIN TIMER: " << ((SonicCharObj2*)Player1->Data2)->SpindashTimer << endl;
		packet << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
		break;

#pragma endregion

#pragma region System

	case MSG_S_2PMODE:	// Not yet implemented.
		return false;

	case MSG_S_2PREADY:
		packet << PlayerReady[0];
		local.menu.PlayerReady[0] = PlayerReady[0];
		break;

	case MSG_S_2PSPECIALS:
		packet.append(P1SpecialAttacks, sizeof(char) * 3);
		memcpy(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3);
		break;

	case MSG_S_BATTLEOPT:
		packet.append(BattleOptions, BattleOptions_Length);
		memcpy(local.menu.BattleOptions, BattleOptions, BattleOptions_Length);
		break;

	case MSG_S_GAMESTATE:
		packet << GameState;
		PrintDebug("<< GameState [%d %d]", local.system.GameState, GameState);
		local.system.GameState = GameState;
		break;

	case MSG_S_LEVEL:	// Not yet implemented
		return false;

	case MSG_S_PAUSESEL:
		packet << PauseSelection;
		local.system.PauseSelection = PauseSelection;
		break;

	case MSG_S_RINGS:
		packet << RingCount[0];
		local.game.RingCount[0] = RingCount[0];
		break;

	case MSG_S_TIME:
		packet << TimerMinutes << TimerSeconds << TimerFrames;
		memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
		break;

	case MSG_S_TIMESTOP:
		PrintDebug("<< Sending Time Stop");

		// Swap the Time Stop value, as this is connected to player number,
		// and Player 1 and 2 are relative to the game instance.
		packet << (char)(TimeStopMode * 5 % 3);

		local.game.TimeStopMode = TimeStopMode;
		break;

#pragma endregion

	}

	return true;
}

#pragma endregion
#pragma region Receive

bool MemoryHandler::ReceiveInput(uint8 type, sf::Packet& packet)
{
	if (CurrentMenu[0] == Menu::BATTLE || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_I_BUTTONS);
			inputLock.lock();
			packet >> recvInput.HeldButtons;
			inputLock.unlock();
			break;

			RECEIVED(MSG_I_ANALOG);
			inputLock.lock();
			packet >> recvInput.LeftStickX >> recvInput.LeftStickY;
			inputLock.unlock();
			break;
		}

		return true;
	}

	return false;
}
bool MemoryHandler::ReceiveSystem(uint8 type, sf::Packet& packet)
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
			break;

			RECEIVED(MSG_S_GAMESTATE);
			{
				uint8 recvGameState;
				packet >> recvGameState;
				if (GameState >= GameState::NORMAL_RESTART && recvGameState > GameState::LOAD_FINISHED)
					GameState = local.system.GameState = recvGameState;

				break;
			}

			RECEIVED(MSG_S_PAUSESEL);
			packet >> local.system.PauseSelection;
			PauseSelection = local.system.PauseSelection;
			break;

			RECEIVED(MSG_S_TIMESTOP);
			packet >> local.game.TimeStopMode;
			writeTimeStop();
			break;

			RECEIVED(MSG_S_2PSPECIALS);
			for (int i = 0; i < 3; i++)
				packet >> local.game.P2SpecialAttacks[i];

			writeSpecials();
			break;

			RECEIVED(MSG_S_RINGS);
			packet >> local.game.RingCount[1];
			writeRings();

			PrintDebug(">> Ring Count Change %d", local.game.RingCount[1]);
			break;
		}

		return true;
	}

	return false;
}
bool MemoryHandler::ReceivePlayer(uint8 type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = (type > MSG_P_START && type < MSG_P_END);

		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_P_ACTION);
			packet >> recvPlayer.Data1.Action;
			break;

			RECEIVED(MSG_P_STATUS);
			packet >> recvPlayer.Data1.Status;
			break;

			RECEIVED(MSG_P_ROTATION);
			packet >> recvPlayer.Data1.Rotation;
			break;

			RECEIVED(MSG_P_POSITION);
			packet >> recvPlayer.Data1.Position;
			break;

			RECEIVED(MSG_P_SCALE);
			packet >> recvPlayer.Data1.Scale;
			break;

			RECEIVED(MSG_P_POWERUPS);
			{
				int powerups;
				packet >> powerups;
				recvPlayer.Data2.Powerups = (Powerups)powerups;
				break;
			}

			RECEIVED(MSG_P_UPGRADES);
			{
				int upgrades;
				packet >> upgrades;
				recvPlayer.Data2.Upgrades = (Upgrades)upgrades;
				break;
			}

			RECEIVED(MSG_P_HP);
			packet >> recvPlayer.Data2.MechHP;
			PrintDebug(">> Received HP update. (%f)", recvPlayer.Data2.MechHP);
			break;

			RECEIVED(MSG_P_SPEED);
			packet >> recvPlayer.Data2.HSpeed;
			packet >> recvPlayer.Data2.VSpeed;
			packet >> recvPlayer.Data2.PhysData.BaseSpeed;
			break;

			RECEIVED(MSG_P_ANIMATION);
			packet >> recvPlayer.Data2.AnimInfo.Next;
			break;

			RECEIVED(MSG_P_SPINTIMER);
			packet >> recvPlayer.Sonic.SpindashTimer;
			break;
		}

		return writePlayer;
	}

	return false;
}
bool MemoryHandler::ReceiveMenu(uint8 type, sf::Packet& packet)
{
	if (GameState == GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_S_2PREADY);
			packet >> local.menu.PlayerReady[1];
			PlayerReady[1] = local.menu.PlayerReady[1];

			PrintDebug(">> Player 2 ready state changed. ", local.menu.PlayerReady[1]);
			break;

			RECEIVED(MSG_M_CHARSEL);
			packet >> local.menu.CharacterSelection[1];
			CharacterSelection[1] = local.menu.CharacterSelection[1];
			//cout << (ushort)local.menu.CharacterSelection[1] << ' ' << (ushort)CharacterSelection[1] << endl;
			break;

			RECEIVED(MSG_M_CHARCHOSEN);
			packet >> local.menu.CharacterSelected[1];
			CharacterSelected[1] = local.menu.CharacterSelected[1];
			//cout << (ushort)CharacterSelected[1] << ' ' << (ushort)local.menu.CharacterSelected[1] << endl;
			break;

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

			break;

			RECEIVED(MSG_S_BATTLEOPT);
			for (int i = 0; i < 4; i++)
				packet >> local.menu.BattleOptions[i];
			memcpy(BattleOptions, local.menu.BattleOptions, sizeof(char) * 4);

			break;

			RECEIVED(MSG_M_BATTLEOPTSEL);
			packet >> local.menu.BattleOptionsSelection
				>> local.menu.BattleOptionsBack;
			BattleOptionsSelection = local.menu.BattleOptionsSelection;
			BattleOptionsBack = local.menu.BattleOptionsBack;

			break;

			RECEIVED(MSG_M_STAGESEL);
			packet >> local.menu.StageSelection2P[0]
				>> local.menu.StageSelection2P[1]
				>> local.menu.BattleOptionsButton;

			StageSelection2P[0] = local.menu.StageSelection2P[0];
			StageSelection2P[1] = local.menu.StageSelection2P[1];
			BattleOptionsButton = local.menu.BattleOptionsButton;

			break;

			RECEIVED(MSG_M_BATTLESEL);
			packet >> local.menu.BattleSelection;
			BattleSelection = local.menu.BattleSelection;

			break;
		}
		
		return true;
	}

	return false;
}


#pragma endregion
#pragma region Crap

void MemoryHandler::PreReceive()
{
	writeRings();
	writeSpecials();

	// HACK: This entire section
	if (GameState >= GameState::INGAME && Player2 != nullptr)
	{
		// HACK: Upgrade/Powerup failsafe
		Player2->Data2->Powerups = recvPlayer.Data2.Powerups;
		Player2->Data2->Upgrades = recvPlayer.Data2.Upgrades;

		// HACK: Mech HP synchronization fix. This REALLY sucks.
		if (Player2->Data2->CharID2 == Characters_MechEggman || Player2->Data2->CharID2 == Characters_MechTails)
			Player2->Data2->MechHP = recvPlayer.Data2.MechHP;

		// HACK: Analog failsafe
		if (GameState == GameState::PAUSE && (recvInput.LeftStickX != 0 || recvInput.LeftStickY != 0))
		{
			recvInput.LeftStickX = 0;
			recvInput.LeftStickY = 0;
		}
	}
}
void MemoryHandler::PostReceive()
{
	writeRings();
	writeSpecials();
}

inline void MemoryHandler::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void MemoryHandler::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void MemoryHandler::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

#pragma endregion
#pragma region Toggles

void MemoryHandler::ToggleSplitscreen()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((ControllersRaw[0].HeldButtons & Buttons_L && ControllersRaw[0].PressedButtons & Buttons_R) ||
			(ControllersRaw[0].PressedButtons & Buttons_L && ControllersRaw[0].HeldButtons & Buttons_R) ||
			(ControllersRaw[0].PressedButtons & Buttons_L && ControllersRaw[0].PressedButtons & Buttons_R))
		{
			if (SplitscreenMode == 1)
				SplitscreenMode = 2;
			else if (SplitscreenMode == 2)
				SplitscreenMode = 1;
		}
	}
}
bool MemoryHandler::Teleport()
{
	if (GameState == GameState::INGAME && TwoPlayerMode > 0)
	{
		if ((ControllersRaw[0].HeldButtons & Buttons_Y) && (ControllersRaw[0].PressedButtons & Buttons_Up))
		{
			// Teleport to recvPlayer
			PrintDebug("<> Teleporting to other player...");;
			PlayerObject::Teleport(&recvPlayer, Player1);
			return true;
		}
	}

	return false;
}

#pragma endregion
