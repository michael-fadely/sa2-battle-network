#include "stdafx.h"

// Defines
#define RECV_VERBOSE(type) case type: PrintDebug(">> [%06d] " #type, FrameCount)
#define RECV_CONCISE(type) case type:

#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

// Standard Includes
#include <cmath>						// for abs
#include <fstream>
#include <thread>

// Local Includes
#include "Globals.h"					// for Globals :specialed:
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketExtensions.h"			// for PacketEx
#include "PacketOverloads.h"
#include "AdventurePacketOverloads.h"

#include <SA2ModLoader.h>
#include "ModLoaderExtensions.h"
#include "AddressList.h"

#include "AddRings.h"
#include "AddHP.h"
#include "HurtPlayer.h"
#include "Random.h"
#include "ItemBoxItems.h"

// This Class
#include "PacketBroker.h"

// Namespaces
using namespace std;
using namespace nethax;

#pragma region science

// TODO: Re-evaluate all this science.
// TODO: Consider using the same timer for all three.

static const float	positionThreshold	= 16.0f;
static const int	rotateThreshold		= NJM_DEG_ANG(11.25);
static const float	speedThreshold		= 0.1f;

static uint positionTimer = 0;
static uint rotateTimer = 0;
static uint speedTimer = 0;

static bool PositionThreshold(NJS_VECTOR& last, NJS_VECTOR& current)
{
	// HACK: Right now this basically means it only sends on a timer.
	return (abs(CheckDistance(&last, &current)) >= positionThreshold || /*memcmp(&last, &current, sizeof(Vertex)) != 0 &&*/ Duration(positionTimer) >= 10000);
}

static bool RotationThreshold(const Rotation& last, const Rotation& current)
{
	return (abs(last.x - current.x) >= rotateThreshold
		|| abs(last.y - current.y) >= rotateThreshold
		|| abs(last.z - current.z) >= rotateThreshold
		|| Duration(rotateTimer) >= 125 && memcmp(&last, &current, sizeof(Rotation)) != 0);
}

static bool SpeedThreshold(NJS_VECTOR& last, NJS_VECTOR& current)
{
	float m = (float)njScalor(&current);
	return (Duration(speedTimer) >= 10000 || abs(CheckDistance(&last, &current)) >= max((speedThreshold * m), speedThreshold));
}

static bool roundStarted()
{
	return GameState > GameState::LoadFinished && TwoPlayerMode && !Pose2PStart_PlayerNum;
}

// TODO: Exclude potentially problematic status bits (i.e DoNextAction, OnPath)
static const ushort status_mask = ~(Status_HoldObject | Status_Unknown1 | Status_Unknown2 | Status_Unknown3 | Status_Unknown4 | Status_Unknown5 | Status_Unknown6);

#pragma endregion

PacketBroker::PacketBroker(uint timeout) : ConnectionTimeout(timeout), netStat(false), tcpPacket(true), udpPacket(false)
{
	Initialize();
}
void PacketBroker::Initialize()
{
	local		= {};
	recvInput	= {};
	recvAnalog	= {};
	sendInput	= {};
	sendAnalog	= {};

	firstMenuEntry	= false;
	wroteP2Start	= false;
	writePlayer		= false;
	timedOut		= false;

	receivedKeepalive = sentKeepalive = 0;
	lastSequence = 0;
	WaitRequests.clear();

	if (netStat)
	{
		sendStats.clear();
		recvStats.clear();

		received_packets = 0;
		received_bytes = 0;
		sent_packets = 0;
		sent_bytes = 0;
	}
}

void PacketBroker::ReceiveLoop()
{
	sf::Packet packet;
	receive(packet, true);
	receive(packet, false);

	timedOut = (Duration(receivedKeepalive) >= ConnectionTimeout);
}

void PacketBroker::receive(sf::Packet& packet, const bool isSafe)
{
	using namespace sf;

	if (isSafe)
	{
		if (Globals::Networking->ReceiveSafe(packet) != Socket::Status::Done)
			return;
	}
	else
	{
		PacketHandler::RemoteAddress remoteAddress;
		if (Globals::Networking->ReceiveFast(packet, remoteAddress) != Socket::Status::Done)
			return;

		if (!Globals::Networking->isConnectedAddress(remoteAddress))
			return;

		ushort sequence = 0;
		packet >> sequence;

		// TODO: Rejection threshold
		if (sequence == 0 || sequence <= lastSequence)
		{
			PrintDebug(">> Received out of order packet. Rejecting.");
			lastSequence = sequence % USHRT_MAX;
			return;
		}

		lastSequence = sequence % USHRT_MAX;
	}

	//MessageID lastType = MessageID::None;

	if (netStat)
		addBytesReceived(packet.getDataSize());

	while (!packet.endOfPacket())
	{
		MessageID newType;
		packet >> newType;

		// TODO: Re-implement packet loop failsafe using read offset.
		/*
		if (newType == lastType)
		{
			PrintDebug("\a<> Packet read loop failsafe! [LAST %d - RECV %d]", lastType, newType);
			break;
		}
		*/

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
				MessageID waitID; packet >> waitID;
				auto it = WaitRequests.find(waitID);

				if (it != WaitRequests.end())
				{
					it->second = true;
				}
				else
				{
					WaitRequests[waitID] = true;
				}

				break;
			}

			case MessageID::S_KeepAlive:
				receivedKeepalive = Millisecs();
				packet.seekRead(sizeof(ushort), SEEK_CUR);
				break;

			default:
			{
				if (newType < MessageID::N_END)
				{
					packet.clear();
					break;
				}

				ushort length;
				packet >> length;


				addTypeReceived(newType, length, isSafe);

				if (receiveInput(newType, packet) || receiveSystem(newType, packet))
					break;

				// HACK: This isn't really a sufficient fix for the scale bug.
				// I suspect it's causing some weird side effects like "falling" while going down a slope,
				// usually interrupting spindashes. However, it fixes the scale issue.
				// (where the scale would be received, but overwritten with 0 before it could be applied to the player due to this function call)
				if (!writePlayer)
					inPlayer.Copy(Player2);

				if (receivePlayer(newType, packet))
				{
					if (GameState >= GameState::Ingame)
					{
						writePlayer = false;
						PlayerObject::WritePlayer(Player2, &inPlayer);
					}

					break;
				}

				if (receiveMenu(newType, packet))
					break;

				PrintDebug("\t\tSkipping %d bytes for id %02d", length, newType);
				packet.seekRead(length, SEEK_CUR);

				break;
			}
		}

		//lastType = newType;
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
	auto it = WaitRequests.find(id);
	if (it == WaitRequests.end())
		it = WaitRequests.insert(it, pair<MessageID, bool>(id, false));

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
	WaitRequests.erase(it);
	return true;
}

void PacketBroker::SendReady(const nethax::MessageID id)
{
	sf::Packet packet;
	packet << MessageID::N_Ready << id;
	Globals::Networking->SendSafe(packet);
}

void PacketBroker::SetConnectTime()
{
	receivedKeepalive = sentKeepalive = Millisecs();
}

void PacketBroker::ToggleNetStat(bool toggle)
{
	netStat = toggle;
}

void PacketBroker::SaveNetStat() const
{
	if (!netStat)
		return;

	ofstream netstat_sent("netstat.sent.csv");
	WriteNetStatCSV(netstat_sent, sendStats);
	netstat_sent << "Total packets/bytes (including overhead): " << sent_packets << '/' << sent_bytes + 4 * sent_packets << endl;

	ofstream netstat_recv("netstat.recv.csv");
	WriteNetStatCSV(netstat_recv, recvStats);
	netstat_recv << "Total packets/bytes (including overhead): " << received_packets << '/' << received_bytes + 4 * received_packets << endl;
}

inline void PacketBroker::addType(nethax::MessageStat& stat, nethax::MessageID id, ushort size, bool isSafe)
{
	if (isSafe)
	{
		++stat.tcpCount;
	}
	else
	{
		++stat.udpCount;
	}

	stat.size = size;
}

void PacketBroker::addTypeReceived(nethax::MessageID id, ushort size, bool isSafe)
{
	if (netStat)
		addType(recvStats[id], id, size, isSafe);
}

void PacketBroker::addTypeSent(nethax::MessageID id, ushort size, bool isSafe)
{
	if (netStat)
		addType(sendStats[id], id, size, isSafe);
}

void PacketBroker::addBytesReceived(size_t size)
{
	if (size)
	{
		++received_packets;
		received_bytes += size;
	}
}

void PacketBroker::addBytesSent(size_t size)
{
	if (size)
	{
		++sent_packets;
		sent_bytes += size;
	}
}

bool PacketBroker::requestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, PacketEx& packetIsIn, bool allowDuplicates)
{
	if (allowDuplicates || !packetIsIn.isInPacket(packetType))
		return requestPacket(packetType, packetAddTo);

	return false;
}
bool PacketBroker::requestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, bool allowDuplicates)
{
	if (packetType >= MessageID::N_Disconnect && packetAddTo.AddType(packetType, allowDuplicates))
		return addPacket(packetType, packetAddTo);

	return false;
}

void PacketBroker::Finalize()
{
	if (netStat)
	{
		addBytesSent(tcpPacket.getDataSize());
		addBytesSent(udpPacket.getDataSize());
	}

	Globals::Networking->Send(tcpPacket);
	Globals::Networking->Send(udpPacket);
	tcpPacket.Clear();
	udpPacket.Clear();
}

#pragma region Send

void PacketBroker::sendSystem(PacketEx& tcp, PacketEx& udp)
{
	if (Duration(sentKeepalive) >= 1000)
		requestPacket(MessageID::S_KeepAlive, udp);

	if (roundStarted())
	{
		if (local.system.GameState != GameState)
			requestPacket(MessageID::S_GameState, tcp, udp);

		if (GameState == GameState::Pause && local.system.PauseSelection != PauseSelection)
			requestPacket(MessageID::S_PauseSelection, tcp, udp);

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking->isServer())
			requestPacket(MessageID::S_Time, udp, tcp);

		if (local.game.TimeStopped != TimeStopped)
			requestPacket(MessageID::S_TimeStop, tcp, udp);

		if (memcmp(local.game.P1SpecialAttacks, P1SpecialAttacks, sizeof(char) * 3) != 0)
			requestPacket(MessageID::S_2PSpecials, tcp, udp);
	}
}
void PacketBroker::sendPlayer(PacketEx& tcp, PacketEx& udp)
{
	if (roundStarted() && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (Globals::Program->InstanceSettings().cheats && GameState == GameState::Ingame && TwoPlayerMode > 0)
		{
			if (ControllerPointers[0]->HeldButtons & Buttons_Y && ControllerPointers[0]->PressedButtons & Buttons_Up)
			{
				// Teleport to recvPlayer
				PrintDebug("<> Teleporting to other player...");;

				Player1->Data1->Position = inPlayer.Data1.Position;
				Player1->Data1->Rotation = inPlayer.Data1.Rotation;
				Player1->Data2->Speed = {};

				requestPacket(MessageID::P_Position, tcp, udp);
				requestPacket(MessageID::P_Speed, tcp, udp);
			}
		}

		char charid = Player1->Data2->CharID2;
		bool sendSpinTimer = 
			charid == Characters_Sonic ||
			charid == Characters_Shadow ||
			charid == Characters_Amy ||
			charid == Characters_MetalSonic;

		if (PositionThreshold(outPlayer.Data1.Position, Player1->Data1->Position))
			requestPacket(MessageID::P_Position, udp, tcp);

		// TODO: Make less spammy
		if (sendSpinTimer && outPlayer.Sonic.SpindashTimer != ((SonicCharObj2*)Player1->Data2)->SpindashTimer)
			requestPacket(MessageID::P_SpinTimer, tcp, udp);

		if (Player1->Data1->Status & Status_DoNextAction && Player1->Data1->NextAction != outPlayer.Data1.NextAction)
			requestPacket(MessageID::P_NextAction, tcp, udp);

		if (outPlayer.Data1.Action != Player1->Data1->Action || (outPlayer.Data1.Status & status_mask) != (Player1->Data1->Status & status_mask))
		{
			requestPacket(MessageID::P_Action, tcp, udp);
			requestPacket(MessageID::P_Status, tcp, udp);

			requestPacket(MessageID::P_Animation, tcp, udp);
			requestPacket(MessageID::P_Position, tcp, udp);

			if (sendSpinTimer)
				requestPacket(MessageID::P_SpinTimer, tcp, udp);
		}

		if (Player1->Data1->Action != 18)
		{
			if (RotationThreshold(outPlayer.Data1.Rotation, Player1->Data1->Rotation)
				|| (SpeedThreshold(outPlayer.Data2.Speed, Player1->Data2->Speed))
				|| outPlayer.Data2.PhysData.BaseSpeed != Player1->Data2->PhysData.BaseSpeed)
			{
				requestPacket(MessageID::P_Rotation, udp, tcp);
				requestPacket(MessageID::P_Position, udp, tcp);
				requestPacket(MessageID::P_Speed, udp, tcp);
			}
		}

		if (memcmp(&outPlayer.Data1.Scale, &Player1->Data1->Scale, sizeof(NJS_VECTOR)) != 0)
			requestPacket(MessageID::P_Scale, tcp, udp);

		if (outPlayer.Data2.Powerups != Player1->Data2->Powerups)
			requestPacket(MessageID::P_Powerups, tcp, udp);

		if (outPlayer.Data2.Upgrades != Player1->Data2->Upgrades)
			requestPacket(MessageID::P_Upgrades, tcp, udp);

		outPlayer.Copy(Player1);
	}
}
void PacketBroker::sendMenu(PacketEx& tcp, PacketEx& udp)
{
	if (GameState == GameState::Inactive && CurrentMenu[0] == Menu::BATTLE)
	{
		// Send battle options
		if (memcmp(local.menu.BattleOptions, BattleOptions, BattleOptions_Length) != 0)
			requestPacket(MessageID::S_BattleOptions, tcp);

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
					requestPacket(MessageID::S_2PReady, tcp);
				break;

			case SubMenu2P::S_BATTLEMODE:
				if (firstMenuEntry || local.menu.BattleSelection != BattleSelection)
					requestPacket(MessageID::M_BattleSelection, tcp);

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
					requestPacket(MessageID::M_CharacterSelection, tcp);
				if (firstMenuEntry || local.menu.CharacterSelected[0] != CharacterSelected[0])
					requestPacket(MessageID::M_CharacterChosen, tcp);

				// I hate this so much
				if (firstMenuEntry || (local.menu.CharSelectThings[0].Costume != CharSelectThings[0].Costume)
					|| (local.menu.CharSelectThings[1].Costume != CharSelectThings[1].Costume)
					|| (local.menu.CharSelectThings[2].Costume != CharSelectThings[2].Costume)
					|| (local.menu.CharSelectThings[3].Costume != CharSelectThings[3].Costume)
					|| (local.menu.CharSelectThings[4].Costume != CharSelectThings[4].Costume)
					|| (local.menu.CharSelectThings[5].Costume != CharSelectThings[5].Costume))
				{
					requestPacket(MessageID::M_CostumeSelection, tcp);
				}

				break;

			case SubMenu2P::S_STAGESEL:
				if (firstMenuEntry
					|| local.menu.StageSelection2P[0] != StageSelection2P[0] || local.menu.StageSelection2P[1] != StageSelection2P[1]
					|| local.menu.BattleOptionsButton != BattleOptionsButton)
				{
					requestPacket(MessageID::M_StageSelection, tcp);
				}
				break;

			case SubMenu2P::S_BATTLEOPT:
				if (firstMenuEntry || local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBack != BattleOptionsBack)
					requestPacket(MessageID::M_BattleConfigSelection, tcp);

				break;
		}
	}
}

bool PacketBroker::addPacket(const nethax::MessageID packetType, PacketEx& packet)
{
	sf::Packet out;

	switch (packetType)
	{
		default:
			return false;

#pragma region Input

		case MessageID::I_Analog:
			out << ControllerPointers[0]->LeftStickX << ControllerPointers[0]->LeftStickY;
			sendInput.LeftStickX = ControllerPointers[0]->LeftStickX;
			sendInput.LeftStickY = ControllerPointers[0]->LeftStickY;
			break;

		case MessageID::I_AnalogAngle:
			out << AnalogThings[0];
			sendAnalog = AnalogThings[0];
			break;

		case MessageID::I_Buttons:
			out << ControllerPointers[0]->HeldButtons;
			sendInput.HeldButtons = ControllerPointers[0]->HeldButtons;
			break;

#pragma endregion

#pragma region Menu

		case MessageID::M_CostumeSelection:
			out << CharSelectThings[0].Costume << CharSelectThings[1].Costume
				<< CharSelectThings[2].Costume << CharSelectThings[3].Costume
				<< CharSelectThings[4].Costume << CharSelectThings[5].Costume;

			local.menu.CharSelectThings[0].Costume = CharSelectThings[0].Costume;
			local.menu.CharSelectThings[1].Costume = CharSelectThings[1].Costume;
			local.menu.CharSelectThings[2].Costume = CharSelectThings[2].Costume;
			local.menu.CharSelectThings[3].Costume = CharSelectThings[3].Costume;
			local.menu.CharSelectThings[4].Costume = CharSelectThings[4].Costume;
			local.menu.CharSelectThings[5].Costume = CharSelectThings[5].Costume;
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

		case MessageID::P_NextAction:
			out << Player1->Data1->NextAction;
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
			PrintDebug("<< HP SEND: %f, %f", Player1->Data2->MechHP, DirtyHPHack);
			out << Player1->Data2->MechHP << DirtyHPHack;
			break;

		case MessageID::P_Speed:
			rotateTimer = speedTimer = Millisecs();
			out << Player1->Data2->Speed << Player1->Data2->PhysData.BaseSpeed;
			break;

		case MessageID::P_Animation:
			out << Player1->Data2->AnimInfo.Next;
			break;

		case MessageID::P_SpinTimer:
			out << ((SonicCharObj2*)Player1->Data2)->SpindashTimer;
			break;

		case MessageID::P_Damage:
			break;

		case MessageID::P_Hurt:
			break;

		case MessageID::P_Kill:
			break;

#pragma endregion

#pragma region System

		case MessageID::S_FrameCount:
			PrintDebug("<< Sending frame count.");
			out << FrameCount;
			break;

		case MessageID::S_Seed:
			PrintDebug("<< Sending seed: 0x%08X", current_seed);
			out << current_seed;
			break;

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
			out << RingCount[0] << DirtyRingHack;
			break;

		case MessageID::S_Time:
			out << TimerMinutes << TimerSeconds << TimerFrames;
			memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			break;

		case MessageID::S_TimeStop:
			PrintDebug("<< Sending Time Stop");

			// Swap the Time Stop value, as this is connected to player number,
			// and Player 1 and 2 are relative to the game instance.
			out << (int8)(TimeStopped * 5 % 3);

			local.game.TimeStopped = TimeStopped;
			break;

		case MessageID::S_Stage:
			PrintDebug("<< Sending stage: %d", CurrentLevel);
			out << CurrentLevel;
			break;

		case MessageID::S_NextStage:
			PrintDebug("<< Sending next stage: %d", NextLevel);
			out << NextLevel;
			break;

		case MessageID::S_NBarrier:
			break;

		case MessageID::S_TBarrier:
			break;

		case MessageID::S_Speedup:
			break;

		case MessageID::S_Invincibility:
			break;

#pragma endregion

	}

	packet << static_cast<ushort>(out.getDataSize());
	packet.append(out.getData(), out.getDataSize());

	addTypeSent(packetType, (ushort)out.getDataSize(), packet.isSafe);

	return true;
}

#pragma endregion
#pragma region Receive

bool PacketBroker::receiveInput(const nethax::MessageID type, sf::Packet& packet)
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

			RECEIVED(MessageID::I_AnalogAngle);
				packet >> recvAnalog;
				break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::receiveSystem(const nethax::MessageID type, sf::Packet& packet)
{
	switch (type)
	{
		default:
			break;

		case MessageID::S_Stage:
			packet >> CurrentLevel;
			return true;

		case MessageID::S_NextStage:
			packet >> NextLevel;
			return true;

		case MessageID::S_FrameCount:
			packet >> FrameCount;
			PrintDebug(">> RECEIVED FRAME COUNT CHANGE: %u", FrameCount);
			return true;

		case MessageID::S_Seed:
			packet >> current_seed;
			PrintDebug(">> Received seed: 0x%08X", current_seed);
			srand_Original(current_seed);
			return true;
	}

	if (roundStarted())
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
				short recvGameState;
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
				packet >> local.game.TimeStopped;
				TimeStopped = local.game.TimeStopped;
				break;

			RECEIVED(MessageID::S_2PSpecials);
				for (int i = 0; i < 3; i++)
					packet >> local.game.P2SpecialAttacks[i];

				SpecialActivateTimer[1] = 60;
				memcpy(P2SpecialAttacks, &local.game.P2SpecialAttacks, sizeof(char) * 3);
				break;

			RECEIVED(MessageID::S_Rings);
			{
				int diff;

				packet >> RingCount[1] >> diff;
				PrintDebug(">> RING CHANGE: %d + %d", RingCount[1], diff);
				AddRingsOriginal(1, diff);

				break;
			}

			case MessageID::S_NBarrier:
				NBarrier_original.Code(nullptr, 1);
				break;

			case MessageID::S_TBarrier:
				TBarrier_original.Code(nullptr, 1);
				break;

			case MessageID::S_Speedup:
				Speedup_original.Code(nullptr, 1);
				break;

			case MessageID::S_Invincibility:
				Invincibility_original.Code(nullptr, 1);
				break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::receivePlayer(const nethax::MessageID type, sf::Packet& packet)
{
	if (roundStarted())
	{
		writePlayer = (type > MessageID::P_START && type < MessageID::P_END);

		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::P_Action);
				packet >> inPlayer.Data1.Action;
				break;

			RECEIVED(MessageID::P_NextAction);
				packet >> inPlayer.Data1.NextAction;
				break;

			RECEIVED(MessageID::P_Status);
				packet >> inPlayer.Data1.Status;
				break;

			RECEIVED(MessageID::P_Rotation);
				packet >> inPlayer.Data1.Rotation;
				break;

			RECEIVED(MessageID::P_Position);
				packet >> inPlayer.Data1.Position;
				break;

			RECEIVED(MessageID::P_Scale);
				packet >> inPlayer.Data1.Scale;
				break;

			RECEIVED(MessageID::P_Powerups);
				packet >> inPlayer.Data2.Powerups;
				break;

			RECEIVED(MessageID::P_Upgrades);
				packet >> inPlayer.Data2.Upgrades;
				break;

			RECEIVED(MessageID::P_HP);
				float hp, diff;
				packet >> hp >> diff;
				PrintDebug(">> HP CHANGE: %f + %f", hp, diff);

				Player2->Data2->MechHP = hp;
				AddHPOriginal(1, diff);
				inPlayer.Data2.MechHP = Player2->Data2->MechHP;
				break;

			RECEIVED(MessageID::P_Speed);
				packet >> inPlayer.Data2.Speed;
				packet >> inPlayer.Data2.PhysData.BaseSpeed;
				break;

			RECEIVED(MessageID::P_Animation);
				packet >> inPlayer.Data2.AnimInfo.Next;
				break;

			RECEIVED(MessageID::P_SpinTimer);
				packet >> inPlayer.Sonic.SpindashTimer;
				break;

			RECEIVED(MessageID::P_Damage);
			{
				do_damage = true;
				break;
			}

			RECEIVED(MessageID::P_Hurt);
			{
				FunctionPointer(void, target, (int playerNum), HurtPlayerHax.Target());
				target(1);
				break;
			}

			RECEIVED(MessageID::P_Kill);
			{
				KillPlayerOriginal(1);
				break;
			}
		}

		return writePlayer;
	}

	return false;
}
bool PacketBroker::receiveMenu(const nethax::MessageID type, sf::Packet& packet)
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

			RECEIVED(MessageID::M_CostumeSelection);
				packet >> local.menu.CharSelectThings[0].Costume
					>> local.menu.CharSelectThings[1].Costume
					>> local.menu.CharSelectThings[2].Costume
					>> local.menu.CharSelectThings[3].Costume
					>> local.menu.CharSelectThings[4].Costume
					>> local.menu.CharSelectThings[5].Costume;

				CharSelectThings[0].Costume = local.menu.CharSelectThings[0].Costume;
				CharSelectThings[1].Costume = local.menu.CharSelectThings[1].Costume;
				CharSelectThings[2].Costume = local.menu.CharSelectThings[2].Costume;
				CharSelectThings[3].Costume = local.menu.CharSelectThings[3].Costume;
				CharSelectThings[4].Costume = local.menu.CharSelectThings[4].Costume;
				CharSelectThings[5].Costume = local.menu.CharSelectThings[5].Costume;

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
