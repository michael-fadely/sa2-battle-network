#include "stdafx.h"

// Defines
#define RECV_VERBOSE(type) case type: PrintDebug(">> [%06d] " #type, FrameCount)
#define RECV_CONCISE(type) case type:

#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

// Standard Includes
#include <cmath>						// for abs

// Local Includes
#include "Globals.h"					// for Globals :specialed:
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketExtensions.h"			// for PacketEx
#include "AdventurePacketOverloads.h"

#include <SA2ModLoader.h>
#include "BAMS.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"

// This Class
#include "PacketBroker.h"

// Namespaces
using namespace std;
using namespace nethax;

#pragma region science

// TODO: Re-evaluate all this science.
// TODO: Consider using the same timer for all three.

static const float	positionThreshold	= 16.0f;
static const int	rotateThreshold		= toBAMS(11.25);
static const float	speedThreshold		= 0.1f;

static uint positionTimer = 0;
static uint rotateTimer = 0;
static uint speedTimer = 0;

static bool PositionThreshold(const Vertex& last, const Vertex& current)
{
	return (fabs(last.x - current.x) >= positionThreshold
		|| fabs(last.y - current.y) >= positionThreshold
		|| fabs(last.z - current.z) >= positionThreshold
		|| /*memcmp(&last, &current, sizeof(Vertex)) != 0 &&*/ Duration(positionTimer) >= 10000);
}

static bool RotationThreshold(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateThreshold
		|| abs(last.y - current.y) >= rotateThreshold
		|| abs(last.z - current.z) >= rotateThreshold
		|| Duration(rotateTimer) >= 125 && memcmp(&last, &current, sizeof(Rotation)) != 0);
}

static bool SpeedThreshold(const float last, const float current)
{
	return last != current && (Duration(speedTimer) >= 10000 || abs(last - current) >= max((speedThreshold * current), speedThreshold));
}

// TODO: Exclude potentially problematic status bits (i.e DoNextAction, OnPath)
static const ushort status_mask = ~(Status_HoldObject | Status_Unknown1 | Status_Unknown2 | Status_Unknown3 | Status_Unknown4 | Status_Unknown5 | Status_Unknown6);

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
	timedOut		= false;

	receivedKeepalive = sentKeepalive = 0;
	lastSequence = 0;
	things.clear();
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
	sf::Socket::Status status = safe ? Globals::Networking->recvSafe(packet) : Globals::Networking->recvFast(packet);

	if (status != sf::Socket::Status::Done)
		return;

	MessageID lastType = MessageID::None;

	if (!safe)
	{
		ushort sequence = 0;
		packet >> sequence;

		// TODO: Threshold
		if (sequence == 0 || sequence <= lastSequence)
		{
			PrintDebug(">> Received out of order packet. Rejecting.");
			return;
		}

		lastSequence = sequence % USHRT_MAX;
	}

	while (!packet.endOfPacket())
	{
		MessageID newType;
		packet >> newType;

		if (newType == lastType)
		{
			PrintDebug("\a<> Packet read loop failsafe! [LAST %d - RECV %d]", lastType, newType);
			break;
		}

		//cout << (ushort)newType << endl;
		switch (newType)
		{
			case MessageID::None:
				PrintDebug("\a>> Reached end of packet.");
				break;

			case MessageID::Count:
				PrintDebug(">> Received message count?! Malformed packet warning!");
				break;

			case MessageID::N_Disconnect:
				PrintDebug(">> Received disconnect request from client.");
				Globals::Networking->Disconnect();
				break;

			case MessageID::N_Ready:
			{
				MessageID id; packet >> id;

				auto it = things.find(id);
				if (it != things.end())
					it->second = true;
				else
					things[id] = true;

				break;
			}

			case MessageID::S_KeepAlive:
				receivedKeepalive = Millisecs();
				packet.seekRead(sizeof(ushort), SEEK_CUR);
				break;

			default:
			{
				// TODO: Consider removing this and treating the key as just a value instead of a message type.
				auto it = things.find(newType);
				if (it != things.end())
					it->second = true;

				if (newType < MessageID::N_END)
				{
					packet.clear();
					break;
				}

				ushort length;
				packet >> length;

				if (ReceiveInput(newType, packet) || ReceiveSystem(newType, packet))
					break;

				// HACK: This isn't really a sufficient fix for the scale bug.
				// I suspect it's causing some weird side effects like "falling" while going down a slope,
				// usually interrupting spindashes. However, it fixes the scale issue.
				// (where the scale would be received, but overwritten with 0 before it could be applied to the player due to this function call)
				if (!writePlayer)
					recvPlayer.Copy(Player2);

				if (ReceivePlayer(newType, packet))
				{
					if (GameState >= GameState::Ingame)
					{
						writePlayer = false;
						PlayerObject::WritePlayer(Player2, &recvPlayer);
					}

					break;
				}

				if (ReceiveMenu(newType, packet))
					break;

				PrintDebug("\t\tSkipping %d bytes for id %02d", length, newType);
				packet.seekRead(length, SEEK_CUR);

				break;
			}
		}

		lastType = newType;
	}
}

bool PacketBroker::ConnectionTimedOut() const
{
#ifdef _DEBUG
	return false;
#else
	return timedOut;
#endif
}

bool PacketBroker::WaitForPlayers(nethax::MessageID id)
{
	auto it = things.find(id);
	if (it == things.end())
		it = things.insert(it, pair<MessageID, bool>(id, false));

	while (!it->second)
	{
		ReceiveLoop();

		if (ConnectionTimedOut())
		{
			PrintDebug("<> Connection timed out while waiting for players.");
			Globals::Program->Disconnect();
			return false;
		}

		this_thread::yield();
	}

	PrintDebug(">> All players ready. Resuming game.");
	things.erase(it);
	return true;
}

void PacketBroker::SendReady(const nethax::MessageID id) const
{
	sf::Packet packet;
	packet << MessageID::N_Ready << id;
	Globals::Networking->sendSafe(packet);
}

void PacketBroker::SetConnectTime()
{
	receivedKeepalive = sentKeepalive = Millisecs();
}

bool PacketBroker::RequestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, PacketEx& packetIsIn)
{
	if (!packetIsIn.isInPacket(packetType))
		return RequestPacket(packetType, packetAddTo);

	return false;
}
bool PacketBroker::RequestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo)
{
	if (packetType >= MessageID::N_Disconnect && packetAddTo.addType(packetType))
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
		RequestPacket(MessageID::S_KeepAlive, fast);

	if (GameState > GameState::LoadFinished && TwoPlayerMode > 0)
	{
		// TODO: Remove local CurrentLevel
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
			RequestPacket(MessageID::S_GameState, safe, fast);

		if (GameState == GameState::Pause && local.system.PauseSelection != PauseSelection)
			RequestPacket(MessageID::S_PauseSelection, safe, fast);

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking->isServer())
			RequestPacket(MessageID::S_Time, fast, safe);

		if (local.game.TimeStopMode != TimeStopMode)
			RequestPacket(MessageID::S_TimeStop, safe, fast);

		if (memcmp(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3) != 0)
			RequestPacket(MessageID::S_2PSpecials, safe, fast);

		if (local.game.RingCount[0] != RingCount[0] && GameState == GameState::Ingame)
			RequestPacket(MessageID::S_Rings, safe, fast);
	}
}
void PacketBroker::SendPlayer(PacketEx& safe, PacketEx& fast)
{
	if (GameState >= GameState::LoadFinished && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (GameState == GameState::Ingame && TwoPlayerMode > 0)
		{
			if ((ControllersRaw[0].HeldButtons & Buttons_Y) && (ControllersRaw[0].PressedButtons & Buttons_Up))
			{
				// Teleport to recvPlayer
				PrintDebug("<> Teleporting to other player...");;

				Player1->Data1->Position = recvPlayer.Data1.Position;
				Player1->Data1->Rotation = recvPlayer.Data1.Rotation;
				Player1->Data2->HSpeed = 0.0f;
				Player1->Data2->VSpeed = 0.0f;

				RequestPacket(MessageID::P_Position, safe, fast);
				RequestPacket(MessageID::P_Speed, safe, fast);
			}
		}

		if (PositionThreshold(sendPlayer.Data1.Position, Player1->Data1->Position))
			RequestPacket(MessageID::P_Position, fast, safe);

		// TODO: Make less spammy
		if (sendSpinTimer && sendPlayer.Sonic.SpindashTimer != ((SonicCharObj2*)Player1->Data2)->SpindashTimer)
			RequestPacket(MessageID::P_SpinTimer, safe, fast);

		if (sendPlayer.Data1.Action != Player1->Data1->Action || (sendPlayer.Data1.Status & status_mask) != (Player1->Data1->Status & status_mask))
		{
			// This "pickup crash fix" only fixed it by NEVER SENDING THE ACTION! Good job, me!
			// sendPlayer.Data1.Action != 0x13 && Player1->Data1->Action == 0x15 || sendPlayer.Data1.Action == 0x20
			RequestPacket(MessageID::P_Action, safe, fast);
			RequestPacket(MessageID::P_Status, safe, fast);

			RequestPacket(MessageID::P_Animation, safe, fast);
			RequestPacket(MessageID::P_Position, safe, fast);

			if (sendSpinTimer)
				RequestPacket(MessageID::P_SpinTimer, safe, fast);
		}

		if (RotationThreshold(sendPlayer.Data1.Rotation, Player1->Data1->Rotation)
			|| (SpeedThreshold(sendPlayer.Data2.HSpeed, Player1->Data2->HSpeed) || SpeedThreshold(sendPlayer.Data2.VSpeed, Player1->Data2->VSpeed))
			|| sendPlayer.Data2.PhysData.BaseSpeed != Player1->Data2->PhysData.BaseSpeed)
		{
			RequestPacket(MessageID::P_Rotation, fast, safe);
			RequestPacket(MessageID::P_Position, fast, safe);
			RequestPacket(MessageID::P_Speed, fast, safe);
		}

		if (memcmp(&sendPlayer.Data1.Scale, &Player1->Data1->Scale, sizeof(Vertex)) != 0)
			RequestPacket(MessageID::P_Scale, safe, fast);

		if (Player1->Data2->CharID == Characters_MechTails || Player1->Data2->CharID == Characters_MechEggman)
		{
			if (sendPlayer.Data2.MechHP != Player1->Data2->MechHP)
				RequestPacket(MessageID::P_HP, safe, fast);
		}

		if (sendPlayer.Data2.Powerups != Player1->Data2->Powerups)
			RequestPacket(MessageID::P_Powerups, safe, fast);

		if (sendPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
			RequestPacket(MessageID::P_Upgrades, safe, fast);

		sendPlayer.Copy(Player1);
	}
}
void PacketBroker::SendMenu(PacketEx& safe, PacketEx& fast)
{
	if (GameState == GameState::Inactive && CurrentMenu[0] == Menu::BATTLE)
	{
		// Send battle options
		if (memcmp(local.menu.BattleOptions, BattleOptions, BattleOptions_Length) != 0)
			RequestPacket(MessageID::S_BattleOptions, safe);

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
					RequestPacket(MessageID::S_2PReady, safe);
				break;

			case SubMenu2P::S_BATTLEMODE:
				if (firstMenuEntry || local.menu.BattleSelection != BattleSelection)
					RequestPacket(MessageID::M_BattleSelection, safe);

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
					RequestPacket(MessageID::M_CharacterSelection, safe);
				if (firstMenuEntry || local.menu.CharacterSelected[0] != CharacterSelected[0])
					RequestPacket(MessageID::M_CharacterChosen, safe);

				// I hate this so much
				if (firstMenuEntry || (local.menu.AltCharacterSonic != AltCharacterSonic)
					|| (local.menu.AltCharacterShadow != AltCharacterShadow)
					|| (local.menu.AltCharacterTails != AltCharacterTails)
					|| (local.menu.AltCharacterEggman != AltCharacterEggman)
					|| (local.menu.AltCharacterKnuckles != AltCharacterKnuckles)
					|| (local.menu.AltCharacterRouge != AltCharacterRouge))
				{
					RequestPacket(MessageID::M_AltCharacter, safe);
				}

				break;

			case SubMenu2P::S_STAGESEL:
				if (firstMenuEntry
					|| local.menu.StageSelection2P[0] != StageSelection2P[0] || local.menu.StageSelection2P[1] != StageSelection2P[1]
					|| local.menu.BattleOptionsButton != BattleOptionsButton)
				{
					RequestPacket(MessageID::M_StageSelection, safe);
				}
				break;

			case SubMenu2P::S_BATTLEOPT:
				if (firstMenuEntry || local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBack != BattleOptionsBack)
					RequestPacket(MessageID::M_BattleConfigSelection, safe);

				break;
		}
	}
}

bool PacketBroker::AddPacket(const nethax::MessageID packetType, PacketEx& packet)
{
	sf::Packet out;

	switch (packetType)
	{
		default:
			return false;

#pragma region Input

		case MessageID::I_Analog:
			out << ControllersRaw[0].LeftStickX << ControllersRaw[0].LeftStickY;
			sendInput.LeftStickX = ControllersRaw[0].LeftStickX;
			sendInput.LeftStickY = ControllersRaw[0].LeftStickY;
			break;

		case MessageID::I_Buttons:
			out << ControllersRaw[0].HeldButtons;
			sendInput.HeldButtons = ControllersRaw[0].HeldButtons;
			break;

#pragma endregion

#pragma region Menu

		case MessageID::M_AltCharacter:
			out << AltCharacterSonic << AltCharacterShadow
				<< AltCharacterTails << AltCharacterEggman
				<< AltCharacterKnuckles << AltCharacterRouge;

			local.menu.AltCharacterSonic = AltCharacterSonic;
			local.menu.AltCharacterShadow = AltCharacterShadow;
			local.menu.AltCharacterTails = AltCharacterTails;
			local.menu.AltCharacterEggman = AltCharacterEggman;
			local.menu.AltCharacterKnuckles = AltCharacterKnuckles;
			local.menu.AltCharacterRouge = AltCharacterRouge;
			break;

		case MessageID::M_BattleSelection:
			out << BattleSelection;
			local.menu.BattleSelection = BattleSelection;
			break;

		case MessageID::M_BattleConfigSelection:
			out << BattleOptionsSelection << BattleOptionsBack;
			local.menu.BattleOptionsSelection = BattleOptionsSelection;
			local.menu.BattleOptionsBack = BattleOptionsBack;
			break;

		case MessageID::M_CharacterChosen:
			out << CharacterSelected[0];
			local.menu.CharacterSelected[0] = CharacterSelected[0];
			break;

		case MessageID::M_CharacterSelection:
			out << CharacterSelection[0];
			local.menu.CharacterSelection[0] = CharacterSelection[0];
			break;

		case MessageID::M_StageSelection:
			out << StageSelection2P[0] << StageSelection2P[1] << BattleOptionsButton;
			local.menu.StageSelection2P[0] = StageSelection2P[0];
			local.menu.StageSelection2P[1] = StageSelection2P[1];
			local.menu.BattleOptionsButton = BattleOptionsButton;
			break;

#pragma endregion

#pragma region Player

			// BEFORE YOU FORGET:
			// The reason sendPlayer is not updated here is because it's done in a separate function all at once.
			// Don't freak out!

		case MessageID::P_Action:
			out << Player1->Data1->Action;
			break;

		case MessageID::P_Status:
			out << Player1->Data1->Status;
			break;

		case MessageID::P_Rotation:
			speedTimer = rotateTimer = Millisecs();
			out << Player1->Data1->Rotation;
			break;

		case MessageID::P_Position:
			// Informs other conditions that it shouldn't request
			// another position out so soon
			positionTimer = Millisecs();
			out << Player1->Data1->Position;
			break;

		case MessageID::P_Scale:
			out << Player1->Data1->Scale;
			break;

		case MessageID::P_Powerups:
			PrintDebug("<< Sending powerups");
			out << Player1->Data2->Powerups;
			break;

		case MessageID::P_Upgrades:
			PrintDebug("<< Sending upgrades");
			out << Player1->Data2->Upgrades;
			break;

		case MessageID::P_HP:
			out << Player1->Data2->MechHP;
			break;

		case MessageID::P_Speed:
			rotateTimer = speedTimer = Millisecs();
			out << Player1->Data2->HSpeed << Player1->Data2->VSpeed << Player1->Data2->PhysData.BaseSpeed;
			break;

		case MessageID::P_Animation:
			out << Player1->Data2->AnimInfo.Next;
			break;

		case MessageID::P_SpinTimer:
			out << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
			break;

#pragma endregion

#pragma region System

		case MessageID::S_KeepAlive:
			sentKeepalive = Millisecs();
			break;

		case MessageID::S_2PReady:
			out << PlayerReady[0];
			local.menu.PlayerReady[0] = PlayerReady[0];
			break;

		case MessageID::S_2PSpecials:
			out.append(P1SpecialAttacks, sizeof(char) * 3);
			memcpy(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3);
			break;

		case MessageID::S_BattleOptions:
			out.append(BattleOptions, BattleOptions_Length);
			memcpy(local.menu.BattleOptions, BattleOptions, BattleOptions_Length);
			break;

		case MessageID::S_GameState:
			out << GameState;
			PrintDebug("<< GameState [%d %d]", local.system.GameState, GameState);
			local.system.GameState = GameState;
			break;

		case MessageID::S_PauseSelection:
			out << PauseSelection;
			local.system.PauseSelection = PauseSelection;
			break;

		case MessageID::S_Rings:
			out << RingCount[0];
			local.game.RingCount[0] = RingCount[0];
			break;

		case MessageID::S_Time:
			out << TimerMinutes << TimerSeconds << TimerFrames;
			memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			break;

		case MessageID::S_TimeStop:
			PrintDebug("<< Sending Time Stop");

			// Swap the Time Stop value, as this is connected to player number,
			// and Player 1 and 2 are relative to the game instance.
			out << (int8)(TimeStopMode * 5 % 3);

			local.game.TimeStopMode = TimeStopMode;
			break;

		case MessageID::S_Stage:
			PrintDebug("<< Sending stage: %d", CurrentLevel);
			out << CurrentLevel;
			break;

#pragma endregion

	}

	packet << static_cast<ushort>(out.getDataSize());
	packet.append(out.getData(), out.getDataSize());

	return true;
}

#pragma endregion
#pragma region Receive

bool PacketBroker::ReceiveInput(const nethax::MessageID type, sf::Packet& packet)
{
	if (CurrentMenu[0] == Menu::BATTLE || TwoPlayerMode > 0 && GameState > GameState::Inactive)
	{
		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::I_Buttons);
				packet >> recvInput.HeldButtons;
				break;

			RECEIVED(MessageID::I_Analog);
				packet >> recvInput.LeftStickX >> recvInput.LeftStickY;
				break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::ReceiveSystem(const nethax::MessageID type, sf::Packet& packet)
{
	if (type == MessageID::S_Stage)
	{
		int level;
		packet >> level;
		CurrentLevel = level;
		return true;
	}

	if (GameState >= GameState::LoadFinished)
	{
		switch (type)
		{
			default:
				return false;

			case MessageID::S_Time:
				packet >> local.game.TimerMinutes >> local.game.TimerSeconds >> local.game.TimerFrames;
				TimerMinutes = local.game.TimerMinutes;
				TimerSeconds = local.game.TimerSeconds;
				TimerFrames = local.game.TimerFrames;
				break;

			RECEIVED(MessageID::S_GameState);
			{
				uint8 recvGameState;
				packet >> recvGameState;
				if (GameState >= GameState::NormalRestart && recvGameState > GameState::LoadFinished)
					GameState = local.system.GameState = recvGameState;

				break;
			}

			RECEIVED(MessageID::S_PauseSelection);
				packet >> local.system.PauseSelection;
				PauseSelection = local.system.PauseSelection;
				break;

			RECEIVED(MessageID::S_TimeStop);
				packet >> local.game.TimeStopMode;
				writeTimeStop();
				break;

			RECEIVED(MessageID::S_2PSpecials);
				for (int i = 0; i < 3; i++)
					packet >> local.game.P2SpecialAttacks[i];

				SpecialActivateTimer[1] = 60;
				writeSpecials();
				break;

			RECEIVED(MessageID::S_Rings);
				packet >> local.game.RingCount[1];
				writeRings();

				PrintDebug(">> Ring Count Change %d", local.game.RingCount[1]);
				break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::ReceivePlayer(const nethax::MessageID type, sf::Packet& packet)
{
	if (GameState >= GameState::LoadFinished)
	{
		writePlayer = (type > MessageID::P_START && type < MessageID::P_END);

		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::P_Action);
				packet >> recvPlayer.Data1.Action;
				break;

			RECEIVED(MessageID::P_Status);
				packet >> recvPlayer.Data1.Status;
				break;

			RECEIVED(MessageID::P_Rotation);
				packet >> recvPlayer.Data1.Rotation;
				break;

			RECEIVED(MessageID::P_Position);
				packet >> recvPlayer.Data1.Position;
				break;

			RECEIVED(MessageID::P_Scale);
				packet >> recvPlayer.Data1.Scale;
				break;

			RECEIVED(MessageID::P_Powerups);
			{
				int powerups = 0;
				packet >> powerups;
				recvPlayer.Data2.Powerups = (Powerups)powerups;
				break;
			}

			RECEIVED(MessageID::P_Upgrades);
			{
				int upgrades = 0;
				packet >> upgrades;
				recvPlayer.Data2.Upgrades = (Upgrades)upgrades;
				break;
			}

			RECEIVED(MessageID::P_HP);
				packet >> recvPlayer.Data2.MechHP;
				PrintDebug(">> Received HP update. (%f)", recvPlayer.Data2.MechHP);
				break;

			RECEIVED(MessageID::P_Speed);
				packet >> recvPlayer.Data2.HSpeed;
				packet >> recvPlayer.Data2.VSpeed;
				packet >> recvPlayer.Data2.PhysData.BaseSpeed;
				break;

			RECEIVED(MessageID::P_Animation);
				packet >> recvPlayer.Data2.AnimInfo.Next;
				break;

			RECEIVED(MessageID::P_SpinTimer);
				packet >> recvPlayer.Sonic.SpindashTimer;
				break;
		}

		return writePlayer;
	}

	return false;
}
bool PacketBroker::ReceiveMenu(const nethax::MessageID type, sf::Packet& packet)
{
	if (GameState == GameState::Inactive)
	{
		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::S_2PReady);
				packet >> local.menu.PlayerReady[1];
				PlayerReady[1] = local.menu.PlayerReady[1];

				PrintDebug(">> Player 2 ready state changed. ", local.menu.PlayerReady[1]);
				break;

			RECEIVED(MessageID::M_CharacterSelection);
				packet >> local.menu.CharacterSelection[1];
				CharacterSelection[1] = local.menu.CharacterSelection[1];
				break;

			RECEIVED(MessageID::M_CharacterChosen);
				packet >> local.menu.CharacterSelected[1];
				CharacterSelected[1] = local.menu.CharacterSelected[1];
				break;

			RECEIVED(MessageID::M_AltCharacter);
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

			RECEIVED(MessageID::S_BattleOptions);
				for (int i = 0; i < 4; i++)
					packet >> local.menu.BattleOptions[i];
				memcpy(BattleOptions, local.menu.BattleOptions, sizeof(char) * 4);

				break;

			RECEIVED(MessageID::M_BattleConfigSelection);
				packet >> local.menu.BattleOptionsSelection
					>> local.menu.BattleOptionsBack;
				BattleOptionsSelection = local.menu.BattleOptionsSelection;
				BattleOptionsBack = local.menu.BattleOptionsBack;

				break;

			RECEIVED(MessageID::M_StageSelection);
				packet >> local.menu.StageSelection2P[0]
					>> local.menu.StageSelection2P[1]
					>> local.menu.BattleOptionsButton;

				StageSelection2P[0] = local.menu.StageSelection2P[0];
				StageSelection2P[1] = local.menu.StageSelection2P[1];
				BattleOptionsButton = local.menu.BattleOptionsButton;

				break;

			RECEIVED(MessageID::M_BattleSelection);
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
	if (GameState >= GameState::Ingame && Player2 != nullptr)
	{
		// HACK: Upgrade/Powerup failsafe
		Player2->Data2->Powerups = recvPlayer.Data2.Powerups;
		Player2->Data2->Upgrades = recvPlayer.Data2.Upgrades;

		// HACK: Mech HP synchronization fix. This REALLY sucks.
		if (Player2->Data2->CharID2 == Characters_MechEggman || Player2->Data2->CharID2 == Characters_MechTails)
			Player2->Data2->MechHP = recvPlayer.Data2.MechHP;

		// HACK: Analog failsafe
		if (GameState == GameState::Pause && (recvInput.LeftStickX != 0 || recvInput.LeftStickY != 0))
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

inline void PacketBroker::writeRings()
{
	RingCount[1] = local.game.RingCount[1];
}
inline void PacketBroker::writeSpecials() const
{
	memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3);
}
inline void PacketBroker::writeTimeStop()
{
	TimeStopMode = local.game.TimeStopMode;
}

#pragma endregion
