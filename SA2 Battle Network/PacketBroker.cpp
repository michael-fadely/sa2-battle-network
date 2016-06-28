#include "stdafx.h"

// Defines
#define RECV_VERBOSE(type) case type: PrintDebug(">> [%06d] " #type, FrameCount)
#define RECV_CONCISE(type) case type:

#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

#define Player MainCharacter

// Standard Includes
#include <cmath> // for abs
#include <fstream>
#include <thread>

// Local Includes
#include "Globals.h"
#include "Common.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketEx.h"
#include "PacketOverloads.h"
#include "AdventurePacketOverloads.h"

#include <SA2ModLoader.h>
#include "ModLoaderExtensions.h"
#include "AddressList.h"

#include "AddHP.h"
#include "OnInput.h"

// This Class
#include "PacketBroker.h"

// Namespaces
using namespace std;
using namespace nethax;

#pragma region science

// TODO: Re-evaluate all this science.
// TODO: Consider using the same timer for all three.

static const float	positionThreshold = 16.0f;
static const int	rotateThreshold   = NJM_DEG_ANG(11.25);
static const float	speedThreshold    = 0.1f;

static uint positionTimer = 0;
static uint rotateTimer   = 0;
static uint speedTimer    = 0;

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

bool roundStarted()
{
	return GameState > GameState::LoadFinished && TwoPlayerMode && !Pose2PStart_PlayerNum;
}

// TODO: Exclude potentially problematic status bits (i.e DoNextAction, OnPath)
static const ushort status_mask = ~(Status_HoldObject | Status_Unknown1 | Status_Unknown2 | Status_Unknown3 | Status_Unknown4 | Status_Unknown5 | Status_Unknown6);

#pragma endregion

PacketBroker::PacketBroker(uint timeout) : ConnectionTimeout(timeout), netStat(false), tcpPacket(Protocol::TCP), udpPacket(Protocol::UDP)
{
	Initialize();
}
void PacketBroker::Initialize()
{
	local = {};

	firstMenuEntry = false;
	writePlayer    = false;
	timedOut       = false;

	sentKeepAlive = 0;
	playerNum = -1;
	sequences.clear();
	keepAlive.clear();
	waitRequests.clear();

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

	auto handler = Globals::Networking;
	for (auto& connection : handler->Connections())
	{
		auto result = handler->ReceiveTCP(packet, connection);

		if (result != sf::Socket::Status::Done)
			continue;

		receive(packet, connection.node, Protocol::TCP);

		if (handler->isServer())
			handler->SendTCP(packet, PacketHandler::BroadcastNode, connection.node);
	}

	PacketHandler::Node node;
	PacketHandler::RemoteAddress remoteAddress;
	auto result = handler->ReceiveUDP(packet, node, remoteAddress);

	if (node >= 0 && result == sf::Socket::Status::Done)
	{
		receive(packet, node, Protocol::UDP);

		if (handler->isServer())
			handler->SendUDP(packet, PacketHandler::BroadcastNode, node);
	}

	auto connections = Globals::Networking->ConnectionCount();
	decltype(connections) timeouts = 0;

	for (auto it = keepAlive.begin(); it != keepAlive.end();)
	{
		if (Duration(it->second) >= ConnectionTimeout)
		{
			PrintDebug("<> Player %d timed out.", it->first);
			sequences.erase(it->first);
			Globals::Networking->Disconnect(it->first);
			it = keepAlive.erase(it);
			++timeouts;
		}
		else
		{
			++it;
		}
	}

	timedOut = timeouts >= connections;
}

bool PacketBroker::Request(MessageID type, Protocol protocol, bool allowDupes)
{
	return request(type,
		protocol == Protocol::TCP ? tcpPacket : udpPacket,
		protocol != Protocol::TCP ? tcpPacket : udpPacket,
		allowDupes);
}

bool PacketBroker::Request(MessageID type, PacketEx& packet, bool allowDupes)
{
	return request(type, packet, allowDupes);
}

bool PacketBroker::Append(MessageID type, Protocol protocol, sf::Packet const* packet, bool allowDupes)
{
	auto& dest = protocol == Protocol::TCP ? tcpPacket : udpPacket;
	auto size = packet == nullptr ? 0 : packet->getDataSize();

	if (!allowDupes && dest.isInPacket(type))
		return false;

	if (!dest.AddType(type, allowDupes))
		return false;

	if (packet != nullptr)
		dest.append(packet->getData(), size);

	dest.Finalize();
	AddTypeSent(type, size, dest.Protocol);
	return true;
}

void PacketBroker::receive(sf::Packet& packet, PacketHandler::Node node, nethax::Protocol protocol)
{
	using namespace sf;

	if (protocol == Protocol::UDP)
	{
		auto& lastSequence = sequences[node];
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

	PlayerNumber pnum = -1;

	if (netStat)
		addBytesReceived(packet.getDataSize());

	while (!packet.endOfPacket())
	{
		MessageID newType;
		packet >> newType;

		// TODO: Re-implement packet loop failsafe using read offset.

		switch (newType)
		{
			case MessageID::None:
				PrintDebug("\a>> Reached end of packet.");
				packet.clear();
				break;

			case MessageID::Count:
				PrintDebug(">> Received message count?! Malformed packet warning!");
				packet.clear();
				break;

			case MessageID::N_Disconnect:
				PrintDebug(">> Player %d disconnected.", node);
				sequences.erase(node);
				keepAlive.erase(node);
				Globals::Networking->Disconnect(node);
				packet.clear();
				break;

			case MessageID::N_Ready:
			{
				PrintDebug(">> Player %d is ready.", node);
				MessageID waitID; packet >> waitID;
				auto it = waitRequests.find(waitID);

				if (it != waitRequests.end())
					++it->second;
				else
					++waitRequests[waitID];

				break;
			}

			case MessageID::N_PlayerNumber:
				packet >> pnum;
				break;

				// TODO: verification that this is from the host
			case MessageID::N_SetPlayer:
			{
				PlayerNumber changed;
				packet >> changed;
				SetPlayerNumber(changed);
				break;
			}

			case MessageID::S_KeepAlive:
				keepAlive[node] = Millisecs();
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

				AddTypeReceived(newType, length, protocol == Protocol::TCP);

				if (pnum >= 0)
				{
					if (receiveSystem(newType, pnum, packet))
						break;

					if (pnum != playerNum)
					{
						if (!writePlayer)
							inPlayer.Copy(Player[pnum]);

						if (receivePlayer(newType, pnum, packet))
						{
							if (GameState >= GameState::Ingame)
							{
								writePlayer = false;
								PlayerObject::WritePlayer(Player[pnum], &inPlayer);
							}

							break;
						}
					}

					if (receiveMenu(newType, pnum, packet))
						break;
				}

				if (runMessageHandler(newType, pnum, packet))
					break;

				PrintDebug("\t\t[P%d] Skipping %d bytes for id %02d", pnum, length, newType);
				packet.seekRead(length, SEEK_CUR);

				break;
			}
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool PacketBroker::ConnectionTimedOut() const
{
#ifdef _DEBUG
	return false;
#else
	return timedOut;
#endif
}

bool PacketBroker::WaitForPlayers(MessageID id)
{
	auto it = waitRequests.find(id);
	if (it == waitRequests.end())
		it = waitRequests.insert(it, pair<MessageID, uint>(id, 0));

	while (it->second < (PlayerNumber)Globals::Networking->ConnectionCount())
	{
		// This handles incrementing of it->second
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
	waitRequests.erase(it);
	return true;
}

void PacketBroker::SendReady(MessageID id)
{
	sf::Packet packet;
	AddReady(id, packet);
	Globals::Networking->SendTCP(packet);
}

void PacketBroker::AddReady(MessageID id, sf::Packet& packet)
{
	AddTypeSent(id, sizeof(MessageID), Protocol::TCP);
	packet << MessageID::N_Ready << id;
}

void PacketBroker::SetConnectTime()
{
	sentKeepAlive = Millisecs();
	for (auto& i : Globals::Networking->Connections())
		keepAlive[i.node] = sentKeepAlive;
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

void PacketBroker::RegisterMessageHandler(MessageID type, MessageHandler func)
{
	messageHandlers[type] = func;
}

void PacketBroker::SetPlayerNumber(PlayerNumber number)
{
	auto from = playerNum < 0 ? 0 : playerNum;
	if (from != number)
		SwapInput(from, number);
	playerNum = number;

	Finalize();
}

inline void PacketBroker::addType(MessageStat& stat, MessageID id, ushort size, bool isSafe)
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

void PacketBroker::AddTypeReceived(MessageID id, size_t size, bool isSafe)
{
	if (netStat)
		addType(recvStats[id], id, (ushort)size, isSafe);
}

void PacketBroker::AddTypeSent(MessageID id, size_t size, Protocol protocol)
{
	if (netStat)
		addType(sendStats[id], id, (ushort)size, protocol == Protocol::TCP);
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

inline bool PacketBroker::request(MessageID type, PacketEx& packet, PacketEx& exclude, bool allowDupes)
{
	if (allowDupes || !exclude.isInPacket(type))
		return request(type, packet);

	return false;
}
bool PacketBroker::request(MessageID type, PacketEx& packet, bool allowDupes)
{
	if (type >= MessageID::N_Disconnect && packet.AddType(type, allowDupes))
		return addPacket(type, packet);

	return false;
}

void PacketBroker::Finalize()
{
	Send(tcpPacket);
	Send(udpPacket);
	tcpPacket.Clear();
	udpPacket.Clear();
	tcpPacket << MessageID::N_PlayerNumber << playerNum;
	udpPacket << MessageID::N_PlayerNumber << playerNum;
}

void PacketBroker::Send(PacketEx& packet)
{
	if (netStat)
		addBytesSent(packet.getDataSize());
	
	Globals::Networking->Send(packet);
}

#pragma region Send

void PacketBroker::sendSystem(PacketEx& tcp, PacketEx& udp)
{
	if (Duration(sentKeepAlive) >= 1000)
		request(MessageID::S_KeepAlive, udp);

	if (roundStarted())
	{
		if (local.system.GameState != GameState)
			request(MessageID::S_GameState, tcp, udp);

		if (GameState == GameState::Pause && local.system.PauseSelection != PauseSelection)
			request(MessageID::S_PauseSelection, tcp, udp);

		if (local.game.TimerSeconds != TimerSeconds && Globals::Networking->isServer())
			request(MessageID::S_Time, udp, tcp);

		if (local.game.TimeStopped != TimeStopped)
			request(MessageID::S_TimeStop, tcp, udp);

		if (memcmp(local.game.SpecialAttacks[playerNum], playerNum == 0 ? P1SpecialAttacks : P2SpecialAttacks, sizeof(char) * 3) != 0)
			request(MessageID::S_2PSpecials, tcp, udp);
	}
}
void PacketBroker::sendPlayer(PacketEx& tcp, PacketEx& udp)
{
	if (roundStarted() && CurrentMenu[0] >= Menu::BATTLE)
	{
		if (Globals::Program->InstanceSettings().cheats && GameState == GameState::Ingame && TwoPlayerMode > 0)
		{
			if (ControllerPointers[playerNum]->HeldButtons & Buttons_Y && ControllerPointers[playerNum]->PressedButtons & Buttons_Up)
			{
				// Teleport to recvPlayer
				PrintDebug("<> Teleporting to other player...");;

				Player[playerNum]->Data1->Position = inPlayer.Data1.Position;
				Player[playerNum]->Data1->Rotation = inPlayer.Data1.Rotation;
				Player[playerNum]->Data2->Speed = {};

				request(MessageID::P_Position, tcp, udp);
				request(MessageID::P_Speed, tcp, udp);
			}
		}

		char charid = Player[playerNum]->Data2->CharID2;
		bool sendSpinTimer = 
			charid == Characters_Sonic ||
			charid == Characters_Shadow ||
			charid == Characters_Amy ||
			charid == Characters_MetalSonic;

		if (PositionThreshold(outPlayer.Data1.Position, Player[playerNum]->Data1->Position))
			request(MessageID::P_Position, udp, tcp);

		// TODO: Make less spammy
		if (sendSpinTimer && outPlayer.Sonic.SpindashTimer != ((SonicCharObj2*)Player[playerNum]->Data2)->SpindashTimer)
			request(MessageID::P_SpinTimer, tcp, udp);

		if (Player[playerNum]->Data1->Status & Status_DoNextAction && Player[playerNum]->Data1->NextAction != outPlayer.Data1.NextAction)
			request(MessageID::P_NextAction, tcp, udp);

		if (outPlayer.Data1.Action != Player[playerNum]->Data1->Action || (outPlayer.Data1.Status & status_mask) != (Player[playerNum]->Data1->Status & status_mask))
		{
			request(MessageID::P_Action, tcp, udp);
			request(MessageID::P_Status, tcp, udp);

			request(MessageID::P_Animation, tcp, udp);
			request(MessageID::P_Position, tcp, udp);

			if (sendSpinTimer)
				request(MessageID::P_SpinTimer, tcp, udp);
		}

		if (Player[playerNum]->Data1->Action != Action_ObjectControl)
		{
			if (RotationThreshold(outPlayer.Data1.Rotation, Player[playerNum]->Data1->Rotation)
				|| (SpeedThreshold(outPlayer.Data2.Speed, Player[playerNum]->Data2->Speed))
				|| outPlayer.Data2.PhysData.BaseSpeed != Player[playerNum]->Data2->PhysData.BaseSpeed)
			{
				request(MessageID::P_Rotation, udp, tcp);
				request(MessageID::P_Position, udp, tcp);
				request(MessageID::P_Speed, udp, tcp);
			}
		}

		if (memcmp(&outPlayer.Data1.Scale, &Player[playerNum]->Data1->Scale, sizeof(NJS_VECTOR)) != 0)
			request(MessageID::P_Scale, tcp, udp);

		if (outPlayer.Data2.Powerups != Player[playerNum]->Data2->Powerups)
			request(MessageID::P_Powerups, tcp, udp);

		if (outPlayer.Data2.Upgrades != Player[playerNum]->Data2->Upgrades)
			request(MessageID::P_Upgrades, tcp, udp);

		outPlayer.Copy(Player[playerNum]);
	}
}
void PacketBroker::sendMenu(PacketEx& tcp, PacketEx& udp)
{
	if (GameState == GameState::Inactive && CurrentMenu[0] == Menu::BATTLE)
	{
		// Send battle options
		if (memcmp(local.menu.BattleOptions, BattleOptions, BattleOptions_Length) != 0)
			request(MessageID::S_BattleOptions, tcp);

		// Always send information about the menu you enter,
		// regardless of detected change.
		if ((firstMenuEntry = (local.menu.SubMenu != CurrentMenu[1] && Globals::Networking->isServer())))
			local.menu.SubMenu = CurrentMenu[1];

		switch (CurrentMenu[1])
		{
			default:
				break;

			case SubMenu2P::S_READY:
			case SubMenu2P::O_READY:
				if (firstMenuEntry || local.menu.PlayerReady[playerNum] != PlayerReady[playerNum])
					request(MessageID::S_2PReady, tcp);
				break;

			case SubMenu2P::S_BATTLEMODE:
				if (firstMenuEntry || local.menu.BattleSelection != BattleSelection)
					request(MessageID::M_BattleSelection, tcp);

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

				if (firstMenuEntry || local.menu.CharacterSelection[playerNum] != CharacterSelection[playerNum])
					request(MessageID::M_CharacterSelection, tcp);
				if (firstMenuEntry || local.menu.CharacterSelected[playerNum] != CharacterSelected[playerNum])
					request(MessageID::M_CharacterChosen, tcp);

				// I hate this so much
				if (firstMenuEntry
					|| local.menu.CharSelectThings[0].Costume != CharSelectThings[0].Costume
					|| local.menu.CharSelectThings[1].Costume != CharSelectThings[1].Costume
					|| local.menu.CharSelectThings[2].Costume != CharSelectThings[2].Costume
					|| local.menu.CharSelectThings[3].Costume != CharSelectThings[3].Costume
					|| local.menu.CharSelectThings[4].Costume != CharSelectThings[4].Costume
					|| local.menu.CharSelectThings[5].Costume != CharSelectThings[5].Costume)
				{
					request(MessageID::M_CostumeSelection, tcp);
				}

				break;

			case SubMenu2P::S_STAGESEL:
				if (firstMenuEntry
					|| local.menu.StageSelection2P[0] != StageSelection2P[0] || local.menu.StageSelection2P[1] != StageSelection2P[1]
					|| local.menu.BattleOptionsButton != BattleOptionsButton)
				{
					request(MessageID::M_StageSelection, tcp);
				}
				break;

			case SubMenu2P::S_BATTLEOPT:
				if (firstMenuEntry || local.menu.BattleOptionsSelection != BattleOptionsSelection || local.menu.BattleOptionsBack != BattleOptionsBack)
					request(MessageID::M_BattleConfigSelection, tcp);

				break;
		}
	}
}

bool PacketBroker::addPacket(MessageID packetType, PacketEx& packet)
{
	switch (packetType)
	{
		default:
			return false;

#pragma region Menu

		case MessageID::M_CostumeSelection:
			packet << CharSelectThings[0].Costume << CharSelectThings[1].Costume
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
			packet << BattleSelection;
			local.menu.BattleSelection = BattleSelection;
			break;

		case MessageID::M_BattleConfigSelection:
			packet << BattleOptionsSelection << BattleOptionsBack;
			local.menu.BattleOptionsSelection = BattleOptionsSelection;
			local.menu.BattleOptionsBack = BattleOptionsBack;
			break;

		case MessageID::M_CharacterChosen:
			packet << CharacterSelected[playerNum];
			local.menu.CharacterSelected[playerNum] = CharacterSelected[playerNum];
			break;

		case MessageID::M_CharacterSelection:
			packet << CharacterSelection[playerNum];
			local.menu.CharacterSelection[playerNum] = CharacterSelection[playerNum];
			break;

		case MessageID::M_StageSelection:
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

		case MessageID::P_Action:
			packet << Player[playerNum]->Data1->Action;
			break;

		case MessageID::P_NextAction:
			packet << Player[playerNum]->Data1->NextAction;
			break;

		case MessageID::P_Status:
			packet << Player[playerNum]->Data1->Status;
			break;

		case MessageID::P_Rotation:
			speedTimer = rotateTimer = Millisecs();
			packet << Player[playerNum]->Data1->Rotation;
			break;

		case MessageID::P_Position:
			// Informs other conditions that it shouldn't request
			// another position out so soon
			positionTimer = Millisecs();
			packet << Player[playerNum]->Data1->Position;
			break;

		case MessageID::P_Scale:
			packet << Player[playerNum]->Data1->Scale;
			break;

		case MessageID::P_Powerups:
			PrintDebug("<< Sending powerups");
			packet << Player[playerNum]->Data2->Powerups;
			break;

		case MessageID::P_Upgrades:
			PrintDebug("<< Sending upgrades");
			packet << Player[playerNum]->Data2->Upgrades;
			break;

		case MessageID::P_Speed:
			rotateTimer = speedTimer = Millisecs();
			packet << Player[playerNum]->Data2->Speed << Player[playerNum]->Data2->PhysData.BaseSpeed;
			break;

		case MessageID::P_Animation:
			packet << Player[playerNum]->Data2->AnimInfo.Next;
			break;

		case MessageID::P_SpinTimer:
			packet << ((SonicCharObj2*)Player[playerNum]->Data2)->SpindashTimer;
			break;

#pragma endregion

#pragma region System

		case MessageID::S_KeepAlive:
			sentKeepAlive = Millisecs();
			break;

		case MessageID::S_2PReady:
			packet << PlayerReady[playerNum];
			local.menu.PlayerReady[playerNum] = PlayerReady[playerNum];
			break;

		case MessageID::S_2PSpecials:
		{
			auto& specials = playerNum == 0 ? P1SpecialAttacks : P2SpecialAttacks;
			packet.append(specials, sizeof(char) * 3);
			memcpy(local.game.SpecialAttacks[playerNum], specials, sizeof(char) * 3);
			break;
		}

		case MessageID::S_BattleOptions:
			packet.append(BattleOptions, BattleOptions_Length);
			memcpy(local.menu.BattleOptions, BattleOptions, BattleOptions_Length);
			break;

		case MessageID::S_GameState:
			packet << GameState;
			PrintDebug("<< GameState [%d %d]", local.system.GameState, GameState);
			local.system.GameState = GameState;
			break;

		case MessageID::S_PauseSelection:
			packet << PauseSelection;
			local.system.PauseSelection = PauseSelection;
			break;

		case MessageID::S_Time:
			packet << TimerMinutes << TimerSeconds << TimerFrames;
			memcpy(&local.game.TimerMinutes, &TimerMinutes, sizeof(char) * 3);
			break;

		case MessageID::S_TimeStop:
			PrintDebug("<< Sending Time Stop");
			packet << TimeStopped;
			local.game.TimeStopped = TimeStopped;
			break;

#pragma endregion

	}

	packet.Finalize();
	AddTypeSent(packetType, packet.GetTypeSize(), packet.Protocol);

	return true;
}

#pragma endregion
#pragma region Receive

// TODO: remove from this class
bool PacketBroker::receiveSystem(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
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
					packet >> local.game.SpecialAttacks[pnum][i];

				SpecialActivateTimer[pnum] = 60;
				memcpy(pnum == 0 ? P1SpecialAttacks : P2SpecialAttacks, &local.game.SpecialAttacks[pnum], sizeof(char) * 3);
				break;
		}

		return true;
	}

	return false;
}
bool PacketBroker::receivePlayer(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	if (roundStarted() && pnum != playerNum)
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

				Player[pnum]->Data2->MechHP = hp;
				events::AddHP_original(pnum, diff);
				inPlayer.Data2.MechHP = Player[pnum]->Data2->MechHP;
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
		}

		return writePlayer;
	}

	return false;
}
bool PacketBroker::receiveMenu(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	if (GameState == GameState::Inactive)
	{
		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::S_2PReady);
				packet >> local.menu.PlayerReady[pnum];
				PlayerReady[pnum] = local.menu.PlayerReady[pnum];

				PrintDebug(">> Player ready state changed. ", local.menu.PlayerReady[pnum]);
				break;

			RECEIVED(MessageID::M_CharacterSelection);
				packet >> local.menu.CharacterSelection[pnum];
				CharacterSelection[pnum] = local.menu.CharacterSelection[pnum];
				break;

			RECEIVED(MessageID::M_CharacterChosen);
				packet >> local.menu.CharacterSelected[pnum];
				CharacterSelected[pnum] = local.menu.CharacterSelected[pnum];
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

bool PacketBroker::runMessageHandler(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	auto it = messageHandlers.find(type);

	if (it == messageHandlers.end())
		return false;

	return it->second(type, pnum, packet);
}

#pragma endregion
