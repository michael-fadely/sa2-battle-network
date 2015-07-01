// Defines
#define RECV_VERBOSE(type) case type: PrintDebug(">> [%06d] " #type, FrameCount)
#define RECV_CONCISE(type) case type:

#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

// Standard Includes
#include <cmath>	// for abs

// Global Includes
#include <LazyTypedefs.h>

// Local Includes
#include "Globals.h"			// for Globals :specialed:
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"			// for MSG
#include "PacketExtensions.h"	// for PacketEx
#include "AdventurePacketOverloads.h"

#include <SA2ModLoader.h>
#include "BAMS.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"

// This Class
#include "PacketBroker.h"

// Namespaces
using namespace std;
using namespace sa2bn;

#pragma region science

// TODO: Re-evaluate all this science.
// TODO: Consider using the same timer for all three.

const float positionThreshold = 16;
const int rotateThreshold = toBAMS(11.25);
const float speedThreshold = 0.1F;

uint positionTimer = 0;
uint rotateTimer = 0;
uint speedTimer = 0;

static inline bool PositionThreshold(const Vertex& last, const Vertex& current)
{
	return (fabs(last.x - current.x) >= positionThreshold
		|| fabs(last.y - current.y) >= positionThreshold
		|| fabs(last.z - current.z) >= positionThreshold
		|| /*memcmp(&last, &current, sizeof(Vertex)) != 0 &&*/ Duration(positionTimer) >= 10000);
}

static inline bool RotationThreshold(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateThreshold
		|| abs(last.y - current.y) >= rotateThreshold
		|| abs(last.z - current.z) >= rotateThreshold
		|| Duration(rotateTimer) >= 125 && memcmp(&last, &current, sizeof(Rotation)) != 0);
}

static inline bool SpeedThreshold(const float last, const float current)
{
	return last != current && (Duration(speedTimer) >= 10000 || abs(last - current) >= max((speedThreshold * current), speedThreshold));
}

#pragma endregion

/*
//	Memory Handler Class
*/

PacketBroker::PacketBroker(uint timeout) : ConnectionTimeout(timeout), safe(true), fast(false)
{
	Initialize();
}
void PacketBroker::Initialize()
{
	local		= {};
	recvInput	= {};
	sendInput	= {};

	firstMenuEntry	= false;
	wroteP2Start	= false;
	writePlayer		= false;
	sendSpinTimer	= false;
	isClientReady	= false;
	stageReceived	= false;
	timedOut		= false;

	receivedKeepalive = sentKeepalive = 0;
}

void PacketBroker::ReceiveLoop()
{
	PreReceive();

	sf::Packet packet;
	Receive(packet, true);
	Receive(packet, false);

	timedOut = (Duration(receivedKeepalive) >= ConnectionTimeout);

	PostReceive();
}

void PacketBroker::Receive(sf::Packet& packet, const bool safe)
{
	sf::Socket::Status status;

	if (safe)
		status = Globals::Networking->recvSafe(packet);
	else
		status = Globals::Networking->recvFast(packet);

	if (status != sf::Socket::Status::Done)
		return;

	uint8 lastType = MSG_NULL;

	while (!packet.endOfPacket())
	{
		uint8 newType;
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

		case MSG_READY:
			isClientReady = true;
			packet.m_readPos += sizeof(ushort);
			break;

		case MSG_S_KEEPALIVE:
			receivedKeepalive = Millisecs();
			packet.m_readPos += sizeof(ushort);
			break;

		default:
			{
				ushort length;
				packet >> length;

				if (ReceiveInput(newType, packet) || ReceiveSystem(newType, packet))
					break;

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

					break;
				}

				if (ReceiveMenu(newType, packet))
					break;

				packet.m_readPos += length;

				break;
			}
		}

		lastType = newType;
	}
}

bool PacketBroker::ConnectionTimedOut() const
{
	return timedOut;
}

bool PacketBroker::WaitForPlayers(bool& condition)
{
	if (!condition)
	{
		PrintDebug("<> Waiting for players...");

		do
		{
			ReceiveLoop();

			if (ConnectionTimedOut())
			{
				PrintDebug("<> Connection timed out while waiting for players.");
				Globals::Program->Disconnect(true);
				return condition = false;
			}

			std::this_thread::yield();
		} while (!condition);

		PrintDebug(">> All players ready. Resuming game.");
	}

	condition = false;
	return true;
}

void PacketBroker::SetConnectTime()
{
	receivedKeepalive = sentKeepalive = Millisecs();
}

bool PacketBroker::RequestPacket(const uint8 packetType, PacketEx& packetAddTo, PacketEx& packetIsIn)
{
	if (!packetIsIn.isInPacket(packetType))
		return RequestPacket(packetType, packetAddTo);

	return false;
}
bool PacketBroker::RequestPacket(const uint8 packetType, PacketEx& packetAddTo)
{
	if (packetType >= MSG_DISCONNECT && packetAddTo.addType(packetType))
		return AddPacket(packetType, packetAddTo);

	return false;
}

void PacketBroker::Finalize()
{
	Globals::Networking->Send(safe);
	Globals::Networking->Send(fast);
	safe.Clear();
	fast.Clear();
}

#pragma region Send

void PacketBroker::SendSystem(PacketEx& safe, PacketEx& fast)
{
	if (Duration(sentKeepalive) >= 1000)
		RequestPacket(MSG_S_KEEPALIVE, fast);

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
void PacketBroker::SendPlayer(PacketEx& safe, PacketEx& fast)
{
	if (GameState >= GameState::LOAD_FINISHED && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (Teleport())
		{
			RequestPacket(MSG_P_POSITION, safe, fast);
			RequestPacket(MSG_P_SPEED, safe, fast);
		}

		if (PositionThreshold(sendPlayer.Data1.Position, Player1->Data1->Position))
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

		if (RotationThreshold(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			|| (SpeedThreshold(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) || SpeedThreshold(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed))
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
void PacketBroker::SendMenu(PacketEx& safe, PacketEx& fast)
{
	if (GameState == GameState::INACTIVE && CurrentMenu[0] == Menu::BATTLE)
	{
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

bool PacketBroker::AddPacket(const uint8 packetType, PacketEx& packet)
{

	auto offset = packet.getDataSize();
	ushort MyCoolShorts = 0;
	packet << MyCoolShorts;

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
		packet << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
		break;

#pragma endregion

#pragma region System

	case MSG_S_KEEPALIVE:
		sentKeepalive = Millisecs();
		break;

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
		packet << (int8)(TimeStopMode * 5 % 3);

		local.game.TimeStopMode = TimeStopMode;
		break;

	case MSG_S_STAGE:
		PrintDebug("<< Sending stage: %d", CurrentLevel);
		packet << CurrentLevel;
		break;

#pragma endregion

	}

	*((ushort*)((uchar*)packet.getData() + offset)) = (packet.getDataSize() - offset);

	return true;
}

#pragma endregion
#pragma region Receive

bool PacketBroker::ReceiveInput(uint8 type, sf::Packet& packet)
{
	if (CurrentMenu[0] == Menu::BATTLE || TwoPlayerMode > 0 && GameState > GameState::INACTIVE)
	{
		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_I_BUTTONS);
			packet >> recvInput.HeldButtons;
			break;

			RECEIVED(MSG_I_ANALOG);
			packet >> recvInput.LeftStickX >> recvInput.LeftStickY;
			break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::ReceiveSystem(uint8 type, sf::Packet& packet)
{
	if (type == MSG_S_STAGE)
	{
		int level;
		packet >> level;
		CurrentLevel = level;

		return stageReceived = true;
	}

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
bool PacketBroker::ReceivePlayer(uint8 type, sf::Packet& packet)
{
	if (GameState >= GameState::LOAD_FINISHED)
	{
		writePlayer = (type > MSG_P_START && type < MSG_P_END);

		switch (type)
		{
		default:
			return false;

			RECEIVED(MSG_P_ACTION);
			// TODO: Add "Do Next Action" to status bits, and also document status bits
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
				int powerups = 0;
				packet >> powerups;
				recvPlayer.Data2.Powerups = (Powerups)powerups;
				break;
			}

			RECEIVED(MSG_P_UPGRADES);
			{
				int upgrades = 0;
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
bool PacketBroker::ReceiveMenu(uint8 type, sf::Packet& packet)
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
			break;

			RECEIVED(MSG_M_CHARCHOSEN);
			packet >> local.menu.CharacterSelected[1];
			CharacterSelected[1] = local.menu.CharacterSelected[1];
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

void PacketBroker::PreReceive()
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
void PacketBroker::PostReceive()
{
	writeRings();
	writeSpecials();
}

inline void PacketBroker::writeRings() { RingCount[1] = local.game.RingCount[1]; }
inline void PacketBroker::writeSpecials() { memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3); }
inline void PacketBroker::writeTimeStop() { TimeStopMode = local.game.TimeStopMode; }

#pragma endregion
#pragma region Toggles

// TODO: Remove from this class
void PacketBroker::ToggleSplitscreen()
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
bool PacketBroker::Teleport()
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
