#include "stdafx.h"

// Defines
#define RECV_VERBOSE(type) case type: PrintDebug(">> [%06d] [P%d] " #type, FrameCount, pnum)
#define RECV_CONCISE(type) case type:

#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

#include <cmath> // for abs
#include <fstream>
#include <thread>

#include "globals.h"
#include "CommonEnums.h"

#include "Networking.h"
#include "PacketEx.h"
#include "AdventurePacketOverloads.h"

#include <SA2ModLoader.h>
#include "ModLoaderExtensions.h"

#include "AddHP.h"
#include "OnInput.h"

#include "PacketBroker.h"


#include "ConnectionManager.h"
#include "reliable.h"

// Namespaces
using namespace std::chrono;
using namespace nethax;

#pragma region science

static const float POSITION_THRESHOLD = 16.0f;
static const int   ROTATE_THRESHOLD   = NJM_DEG_ANG(11.25);
static const float SPEED_THRESHOLD    = 0.1f;

static system_clock::time_point position_timer;
static system_clock::time_point rotate_timer;
static system_clock::time_point speed_timer;

static const auto POSITION_INTERVAL  = milliseconds(10000);
static const auto ROTATION_INTERVAL  = milliseconds(125);
static const auto SPEED_INTERVAL     = milliseconds(10000);
static const auto KEEPALIVE_INTERVAL = milliseconds(1000);

static bool position_threshold(NJS_VECTOR& last, NJS_VECTOR& current)
{
	// HACK: Right now this basically means it only sends on a timer.
	return abs(CheckDistance(&last, &current)) >= POSITION_THRESHOLD ||
	       /*memcmp(&last, &current, sizeof(Vertex)) != 0 &&*/ system_clock::now() - position_timer >= POSITION_INTERVAL;
}

static bool rotation_threshold(const Rotation& last, const Rotation& current)
{
	return abs(last.x - current.x) >= ROTATE_THRESHOLD ||
	       abs(last.y - current.y) >= ROTATE_THRESHOLD ||
	       abs(last.z - current.z) >= ROTATE_THRESHOLD ||
	       (system_clock::now() - rotate_timer >= ROTATION_INTERVAL && memcmp(&last, &current, sizeof(Rotation)) != 0);
}

static bool speed_threshold(NJS_VECTOR& last, NJS_VECTOR& current)
{
	const auto m = static_cast<float>(njScalor(&current));
	return system_clock::now() - speed_timer >= SPEED_INTERVAL ||
	       abs(CheckDistance(&last, &current)) >= std::max((SPEED_THRESHOLD * m), SPEED_THRESHOLD);
}

bool round_started()
{
	return GameState > GameState::LoadFinished && TwoPlayerMode && !Pose2PStart_PlayerNum;
}

// TODO: Exclude potentially problematic status bits (i.e DoNextAction, OnPath)
static const uint STATUS_MASK = ~(Status_HoldObject | Status_Unknown1 | Status_Unknown2 | Status_Unknown3 |
                                  Status_Unknown4 | Status_Unknown5 | Status_Unknown6);

#pragma endregion

PacketBroker::PacketBroker(uint timeout)
	: connection_timeout(milliseconds(timeout)),
	  netstat(false),
	  tcp_packet(Protocol::tcp),
	  udp_packet(Protocol::udp)
{
	initialize();
}

void PacketBroker::initialize()
{
	local = {};

	first_menu_entry = false;
	write_player     = false;
	timed_out        = false;

	sent_keep_alive = system_clock::now();
	player_num      = -1;
	keep_alive.clear();
	wait_requests.clear();

	for (auto& i : net_player)
	{
		i = {};
	}

	if (netstat)
	{
		send_stats.clear();
		recv_stats.clear();

		received_packets = 0;
		received_bytes   = 0;
		sent_packets     = 0;
		sent_bytes       = 0;
	}

	connection_manager_ = std::make_shared<ConnectionManager>();
	connection_nodes_.clear();
	node_connections_.clear();
}

sws::SocketState PacketBroker::listen(const sws::Address& address, std::shared_ptr<Connection>* out_connection)
{
	if (!is_bound())
	{
		PrintDebug("Hosting server on port %d...", address.port);
		set_player_number(0);
		is_server_ = true;
	}

	if (connection_manager_->host(address) != sws::SocketState::done)
	{
		throw std::runtime_error("host failed");
	}

	sws::SocketState result;
	std::shared_ptr<Connection> connection;

	if ((result = connection_manager_->listen(&connection)) == sws::SocketState::done)
	{
		const node_t assigned_node = get_free_node();

		if (assigned_node < 0)
		{
			return sws::SocketState::error;
		}

		connection_nodes_[connection.get()] = assigned_node;
		node_connections_[assigned_node] = connection;

		if (out_connection)
		{
			*out_connection = std::move(connection);
		}
	}

	return result;
}

sws::SocketState PacketBroker::connect(const sws::Address& address, std::shared_ptr<Connection>* out_connection)
{
	if (is_connected())
	{
		return sws::SocketState::done;
	}

	std::shared_ptr<Connection> connection;
	const sws::SocketState result = connection_manager_->connect(address, &connection);

	if (result == sws::SocketState::done)
	{
		constexpr node_t server_node = 0;
		connection_nodes_[connection.get()] = server_node;
		node_connections_[server_node] = connection;

		if (out_connection)
		{
			*out_connection = std::move(connection);
		}
	}

	return result;
}

void PacketBroker::receive_loop()
{
	const bool should_broadcast = is_server_ && is_connected();

	connection_manager_->receive(false);

	std::vector connections(node_connections_.begin(), node_connections_.end());

	for (auto& [node, connection] : connections)
	{
		connection->update();

		sws::Packet received;

		while (connection->pop(&received))
		{
			read(received, node);

			if (!should_broadcast)
			{
				continue;
			}
			
			for (auto& [node_out, connection_out] : connections)
			{
				if (connection_out == connection)
				{
					continue;
				}

				// TODO: check result for connection failure
				connection_out->send(received);
			}
		}
	}

	const size_t connection_count = node_connections_.size();
	size_t timeouts = 0;

#ifndef _DEBUG
	for (auto it = keep_alive.begin(); it != keep_alive.end();)
	{
		if ((system_clock::now() - it->second) >= connection_timeout)
		{
			PrintDebug("<> Player %d timed out.", it->first);
			sequences.erase(it->first);
			disconnect(it->first);
			it = keep_alive.erase(it);
			++timeouts;
		}
		else
		{
			++it;
		}
	}
#endif

	timed_out = timeouts >= connection_count;
}

void PacketBroker::read(sws::Packet& packet, node_t node)
{
	node_t real_node = node;

	if (!is_server_ && packet.work_size() >= sizeof(MessageID) + sizeof(node_t))
	{
		MessageID type;
		packet >> type;

		if (type != MessageID::N_Node)
		{
			packet.seek(sws::SeekCursor::read, sws::SeekType::from_start, 0);
		}
		else
		{
			packet >> real_node;
		}
	}

	pnum_t pnum = -1;

	if (netstat)
	{
		add_bytes_received(packet.real_size());
	}

	while (!packet.end())
	{
		MessageID new_type = MessageID::None;
		packet >> new_type;

		switch (new_type)
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
				PrintDebug(">> Player %d disconnected.", real_node);
				keep_alive.erase(real_node);

				if (is_server_ || real_node == 0)
				{
					disconnect(node);
					packet.clear();
				}
				break;

			case MessageID::N_Ready:
			{
				PrintDebug(">> Player %d is ready.", real_node);
				MessageID wait_id;
				packet >> wait_id;

				if (is_server_ || real_node == 0)
				{
					auto it = wait_requests.find(wait_id);

					if (it != wait_requests.end())
					{
						++it->second.count;
					}
					else if (is_server_)
					{
						++wait_requests[wait_id].count;
					}
				}

				break;
			}

			case MessageID::N_PlayerNumber:
				packet >> pnum;
				break;

			case MessageID::N_SetPlayerNumber:
			{
				if (real_node != 0)
				{
					break;
				}

				pnum_t changed;
				packet >> changed;
				set_player_number(changed);
				break;
			}

			case MessageID::S_KeepAlive:
				keep_alive[node] = system_clock::now();
				packet.seek(sws::SeekCursor::read, sws::SeekType::relative, sizeof(ushort));
				break;

			default:
			{
				if (new_type < MessageID::N_END)
				{
					packet.clear();
					break;
				}

				ushort length = 0;
				packet >> length;

				add_type_received(new_type, length, /* FIXME: protocol */ false);

				if (pnum >= 0 && pnum < 2)
				{
					if (receive_system(new_type, pnum, packet))
					{
						break;
					}

					if (pnum != player_num)
					{
						if (!write_player)
						{
							net_player[pnum].copy(MainCharacter[pnum]);
						}

						if (receive_player(new_type, pnum, packet))
						{
							if (GameState >= GameState::Ingame)
							{
								write_player = false;
								PlayerObject::write_player(MainCharacter[pnum], &net_player[pnum]);
							}

							break;
						}
					}

					if (receive_menu(new_type, pnum, packet))
					{
						break;
					}
				}

				if (run_message_reader(new_type, pnum, packet))
				{
					break;
				}

				PrintDebug("\t\t[P%d] Skipping %d bytes for id %02d", pnum, length, new_type);
				packet.seek(sws::SeekCursor::read, sws::SeekType::relative, length);
				break;
			}
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool PacketBroker::connection_timed_out() const
{
#ifdef _DEBUG
	return false;
#else
	return timed_out;
#endif
}

bool PacketBroker::wait_for_players(MessageID id)
{
	auto it = wait_requests.find(id);

	if (it == wait_requests.end())
	{
		WaitRequest a {};
		it = wait_requests.insert(it, std::make_pair(id, a));
	}

	while (it->second.count < static_cast<pnum_t>(node_connections_.size()))
	{
		// This handles incrementing of it->second.count
		receive_loop();

		if (connection_timed_out())
		{
			PrintDebug("<> Connection timed out while waiting for players.");
			globals::program->disconnect();
			return false;
		}

		std::this_thread::yield();
	}

	PrintDebug(">> All players ready. Resuming game.");
	wait_requests.erase(it);
	return true;
}

void PacketBroker::send_ready(MessageID id)
{
	sws::Packet packet = reliable::reserve(reliable::reliable_t::ordered);
	add_ready(id, packet);

	for (auto& [node, connection] : node_connections_)
	{
		connection->send(packet);
	}
}

bool PacketBroker::send_ready_and_wait(MessageID id)
{
	if (is_server_)
	{
		const bool result = wait_for_players(id);

		if (result)
		{
			send_ready(id);
		}

		return result;
	}

	send_ready(id);
	return wait_for_players(id);
}

void PacketBroker::add_ready(MessageID id, sws::Packet& packet)
{
	add_type_sent(id, sizeof(MessageID), Protocol::tcp);
	packet << MessageID::N_Ready << id;
}

void PacketBroker::toggle_netstat(bool value)
{
	netstat = value;
}

void PacketBroker::save_netstat() const
{
	if (!netstat)
	{
		return;
	}

	std::ofstream netstat_sent("netstat.sent.csv");
	write_netstat_csv(netstat_sent, send_stats);
	netstat_sent << "Total packets/bytes (including overhead): "
		<< sent_packets << '/' << sent_bytes + 4 * sent_packets << std::endl;

	std::ofstream netstat_recv("netstat.recv.csv");
	write_netstat_csv(netstat_recv, recv_stats);
	netstat_recv << "Total packets/bytes (including overhead): "
		<< received_packets << '/' << received_bytes + 4 * received_packets << std::endl;
}

void PacketBroker::register_reader(nethax::MessageID message_id, const MessageReader& reader)
{
	message_readers[message_id] = reader;
}

void PacketBroker::register_writer(nethax::MessageID message_id, const MessageWriter& writer)
{
	message_writers[message_id] = writer;
}

void PacketBroker::set_player_number(pnum_t number)
{
	const pnum_t from = player_num < 0 ? (pnum_t)0 : player_num;

	if (from != number)
	{
		swap_input(from, number);
	}

	player_num = number;

	finalize();
}

bool PacketBroker::is_connected() const
{
	return !connection_nodes_.empty();
}

bool PacketBroker::is_server() const
{
	return is_server_;
}

bool PacketBroker::is_bound() const
{
	return connection_manager_->is_bound();
}

size_t PacketBroker::connection_count() const
{
	return connection_manager_->connection_count();
}

void PacketBroker::disconnect() const
{
	connection_manager_->disconnect();
}

std::shared_ptr<ConnectionManager> PacketBroker::connection_manager() const
{
	return connection_manager_;
}

bool PacketBroker::request(MessageID type, Protocol protocol, bool allow_dupes)
{
	return request(type,
	               protocol == Protocol::tcp ? tcp_packet : udp_packet,
	               protocol != Protocol::tcp ? tcp_packet : udp_packet,
	               allow_dupes);
}

bool PacketBroker::append(MessageID type, Protocol protocol, sws::Packet const* packet, bool allow_dupes)
{
	auto& dest = protocol == Protocol::tcp ? tcp_packet : udp_packet;
	const auto size = packet == nullptr ? 0 : packet->real_size();

	if (!allow_dupes && dest.contains(type))
	{
		return false;
	}

	if (!dest.add_type(type, allow_dupes))
	{
		return false;
	}

	if (packet != nullptr)
	{
		dest << *packet;
	}

	dest.finalize();
	add_type_sent(type, size, dest.protocol);
	return true;
}

inline void PacketBroker::add_type(MessageStat& stat, ushort size, bool is_safe)
{
	if (is_safe)
	{
		++stat.tcp_count;
	}
	else
	{
		++stat.udp_count;
	}

	stat.size = size;
}

void PacketBroker::add_type_received(MessageID id, size_t size, bool is_safe)
{
	if (netstat)
	{
		add_type(recv_stats[id], static_cast<ushort>(size), is_safe);
	}
}

void PacketBroker::add_type_sent(MessageID id, size_t size, Protocol protocol)
{
	if (netstat)
	{
		add_type(send_stats[id], static_cast<ushort>(size), protocol == Protocol::tcp);
	}
}

void PacketBroker::add_bytes_received(size_t size)
{
	if (size)
	{
		++received_packets;
		received_bytes += size;
	}
}

void PacketBroker::add_bytes_sent(size_t size)
{
	if (size)
	{
		++sent_packets;
		sent_bytes += size;
	}
}

node_t PacketBroker::get_free_node()
{
	if (node_connections_.empty())
	{
		return 1;
	}

	node_t last_node = -1;

	for (const auto& pair : node_connections_)
	{
		const node_t node = pair.first;

		if (last_node < 0)
		{
			last_node = node;
			continue;
		}

		if (node - last_node > 1)
		{
			return node + static_cast<node_t>(1);
		}

		last_node = node;
	}

	return -1;
}

bool PacketBroker::request(MessageID type, PacketEx& packet, PacketEx& exclude, bool allow_dupes)
{
	if (allow_dupes || !exclude.contains(type))
	{
		return request(type, packet);
	}

	return false;
}

bool PacketBroker::request(MessageID type, PacketEx& packet, bool allow_dupes)
{
	if (type >= MessageID::N_Disconnect && packet.add_type(type, allow_dupes))
	{
		return add_packet(type, packet);
	}

	return false;
}

void PacketBroker::finalize()
{
	send(tcp_packet);
	send(udp_packet);
	tcp_packet.clear();
	udp_packet.clear();
	tcp_packet << MessageID::N_PlayerNumber << player_num;
	udp_packet << MessageID::N_PlayerNumber << player_num;
}

void PacketBroker::send(sws::Packet& packet, bool block)
{
	if (netstat)
	{
		add_bytes_sent(packet.real_size());
	}

	for (auto& [node, connection] : node_connections_)
	{
		connection->send(packet, block);
	}
}

void PacketBroker::send_system()
{
	send_system(tcp_packet, udp_packet);
}

void PacketBroker::send_player()
{
	send_player(tcp_packet, udp_packet);
}

void PacketBroker::send_menu()
{
	send_menu(tcp_packet);
}

#pragma region Send

void PacketBroker::send_system(PacketEx& tcp, PacketEx& udp)
{
	if ((system_clock::now() - sent_keep_alive) >= KEEPALIVE_INTERVAL)
	{
		request(MessageID::S_KeepAlive, udp);
	}

	// TODO: check if spectator
	if (player_num > 1)
	{
		return;
	}

	if (round_started())
	{
		if (local.system.GameState != GameState)
		{
			request(MessageID::S_GameState, tcp, udp);
		}

		if (GameState == GameState::Pause && local.system.PauseSelection != PauseSelection)
		{
			request(MessageID::S_PauseSelection, tcp, udp);
		}

		if (local.game.TimerSeconds != TimerSeconds && is_server_)
		{
			request(MessageID::S_Time, udp, tcp);
		}

		if (local.game.TimeStopped != TimeStopped)
		{
			request(MessageID::S_TimeStop, tcp, udp);
		}

		if (memcmp(local.game.SpecialAttacks[player_num],
		           player_num == 0 ? P1SpecialAttacks : P2SpecialAttacks, sizeof(char) * 3) != 0)
		{
			request(MessageID::S_2PSpecials, tcp, udp);
		}
	}
}

void PacketBroker::send_player(PacketEx& tcp, PacketEx& udp)
{
	// TODO: check if spectator
	if (player_num > 1)
	{
		return;
	}

	if (round_started() && CurrentMenu >= Menu::battle)
	{
		if (globals::program->settings().cheats && GameState == GameState::Ingame && TwoPlayerMode > 0)
		{
			if (ControllerPointers[player_num]->on & Buttons_Y && ControllerPointers[player_num]->press & Buttons_Up)
			{
				// Teleport to recvPlayer
				PrintDebug("<> Teleporting to other player...");

				auto n = static_cast<int>(player_num != 1);

				MainCharacter[player_num]->Data1.Entity->Position = net_player[n].data1.Position;
				MainCharacter[player_num]->Data1.Entity->Rotation = net_player[n].data1.Rotation;
				MainCharacter[player_num]->Data2.Character->Speed = {};

				request(MessageID::P_Position, tcp, udp);
				request(MessageID::P_Speed, tcp, udp);
			}
		}

		char character_id = MainCharacter[player_num]->Data2.Character->CharID2;

		bool send_spin_timer =
			character_id == Characters_Sonic ||
			character_id == Characters_Shadow ||
			character_id == Characters_Amy ||
			character_id == Characters_MetalSonic;

		if (position_threshold(net_player[player_num].data1.Position, MainCharacter[player_num]->Data1.Entity->Position))
		{
			request(MessageID::P_Position, udp, tcp);
		}

		// TODO: Make less spammy
		if (send_spin_timer && net_player[player_num].sonic.SpindashCounter
		    != reinterpret_cast<SonicCharObj2*>(MainCharacter[player_num]->Data2.Undefined)->SpindashCounter)
		{
			request(MessageID::P_SpinTimer, tcp, udp);
		}

		if (MainCharacter[player_num]->Data1.Entity->Status & Status_DoNextAction && MainCharacter[player_num]->Data1.Entity->NextAction
		    != net_player[player_num].data1.NextAction)
		{
			request(MessageID::P_NextAction, tcp, udp);
		}

		if (net_player[player_num].data1.Action != MainCharacter[player_num]->Data1.Entity->Action ||
		    (net_player[player_num].data1.Status & STATUS_MASK) != (MainCharacter[player_num]->Data1.Entity->Status & STATUS_MASK))
		{
			request(MessageID::P_Action, tcp, udp);
			request(MessageID::P_Status, tcp, udp);

			request(MessageID::P_Animation, tcp, udp);
			request(MessageID::P_Position, tcp, udp);

			if (send_spin_timer)
			{
				request(MessageID::P_SpinTimer, tcp, udp);
			}
		}

		if (MainCharacter[player_num]->Data1.Entity->Action != Action_ObjectControl)
		{
			if (rotation_threshold(net_player[player_num].data1.Rotation, MainCharacter[player_num]->Data1.Entity->Rotation) ||
			    speed_threshold(net_player[player_num].data2.Speed, MainCharacter[player_num]->Data2.Character->Speed) ||
			    net_player[player_num].data2.PhysData.BaseSpeed != MainCharacter[player_num]->Data2.Character->PhysData.BaseSpeed)
			{
				request(MessageID::P_Rotation, udp, tcp);
				request(MessageID::P_Position, udp, tcp);
				request(MessageID::P_Speed, udp, tcp);
			}
		}

		if (memcmp(&net_player[player_num].data1.Scale, &MainCharacter[player_num]->Data1.Entity->Scale, sizeof(NJS_VECTOR)) != 0)
		{
			request(MessageID::P_Scale, tcp, udp);
		}

		if (net_player[player_num].data2.Powerups != MainCharacter[player_num]->Data2.Character->Powerups)
		{
			request(MessageID::P_Powerups, tcp, udp);
		}

		if (net_player[player_num].data2.Upgrades != MainCharacter[player_num]->Data2.Character->Upgrades)
		{
			request(MessageID::P_Upgrades, tcp, udp);
		}

		net_player[player_num].copy(MainCharacter[player_num]);
	}
}

void PacketBroker::send_menu(PacketEx& packet)
{
	// TODO: check if spectator
	if (player_num >= 1)
	{
		return;
	}

	if (GameState == GameState::Inactive && CurrentMenu == Menu::battle)
	{
		// Send battle options
		if (memcmp(local.menu.BattleOptions, BattleOptions, BattleOptions_Length) != 0)
		{
			request(MessageID::S_BattleOptions, packet);
		}

		// Always send information about the menu you enter,
		// regardless of detected change.
		if ((first_menu_entry = (local.menu.SubMenu != CurrentSubMenu && is_server_)))
		{
			local.menu.SubMenu = CurrentSubMenu;
		}

		switch (CurrentSubMenu)
		{
			default:
				break;

			case SubMenu2P::s_ready:
			case SubMenu2P::o_ready:
				if (first_menu_entry || local.menu.PlayerReady[player_num] != PlayerReady[player_num])
				{
					request(MessageID::S_2PReady, packet);
				}

				break;

			case SubMenu2P::s_battlemode:
				if (first_menu_entry || local.menu.BattleSelection != BattleSelection)
				{
					request(MessageID::M_BattleSelection, packet);
				}

				break;

			case SubMenu2P::s_charsel:
			case SubMenu2P::o_charsel:
				// HACK: Character select bug work-around. Details below.
				// When a button press is missed but the character selected state is synchronized,
				// the sub menu does not change to O_CHARSEL, so it won't progress. This forces it to.
				if (CharacterSelected[0] && CharacterSelected[1] && CurrentSubMenu == SubMenu2P::s_charsel)
				{
					PrintDebug("<> Resetting character selections");
					CharacterSelectTimer = 0;
					CurrentSubMenu = SubMenu2P::o_charsel;
				}

				if (first_menu_entry || local.menu.CharacterSelection[player_num] != CharacterSelection[player_num])
				{
					request(MessageID::M_CharacterSelection, packet);
				}

				if (first_menu_entry || local.menu.CharacterSelected[player_num] != CharacterSelected[player_num])
				{
					request(MessageID::M_CharacterChosen, packet);
				}

				// I hate this so much
				if (first_menu_entry
				    || local.menu.CharSelectThings[0].Costume != CharSelectThings[0].Costume
				    || local.menu.CharSelectThings[1].Costume != CharSelectThings[1].Costume
				    || local.menu.CharSelectThings[2].Costume != CharSelectThings[2].Costume
				    || local.menu.CharSelectThings[3].Costume != CharSelectThings[3].Costume
				    || local.menu.CharSelectThings[4].Costume != CharSelectThings[4].Costume
				    || local.menu.CharSelectThings[5].Costume != CharSelectThings[5].Costume)
				{
					request(MessageID::M_CostumeSelection, packet);
				}

				break;

			case SubMenu2P::s_stagesel:
				if (first_menu_entry
				    || local.menu.StageSelection2P[0] != StageSelection2P[0] || local.menu.StageSelection2P[1] != StageSelection2P[1]
				    || local.menu.BattleOptionsButton != BattleOptionsButton)
				{
					request(MessageID::M_StageSelection, packet);
				}
				break;

			case SubMenu2P::s_battleopt:
				if (first_menu_entry || local.menu.BattleOptionsSelection != BattleOptionsSelection
				    || local.menu.BattleOptionsBack != BattleOptionsBack)
				{
					request(MessageID::M_BattleConfigSelection, packet);
				}

				break;
		}
	}
}

bool PacketBroker::add_packet(MessageID packet_type, PacketEx& packet)
{
	switch (packet_type)
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
			local.menu.BattleOptionsBack      = BattleOptionsBack;
			break;

		case MessageID::M_CharacterChosen:
			packet << CharacterSelected[player_num];
			local.menu.CharacterSelected[player_num] = CharacterSelected[player_num];
			break;

		case MessageID::M_CharacterSelection:
			packet << CharacterSelection[player_num];
			local.menu.CharacterSelection[player_num] = CharacterSelection[player_num];
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
			packet << MainCharacter[player_num]->Data1.Entity->Action;
			break;

		case MessageID::P_NextAction:
			packet << MainCharacter[player_num]->Data1.Entity->NextAction;
			break;

		case MessageID::P_Status:
			packet << MainCharacter[player_num]->Data1.Entity->Status;
			break;

		case MessageID::P_Rotation:
			speed_timer = rotate_timer = system_clock::now();
			packet << MainCharacter[player_num]->Data1.Entity->Rotation;
			break;

		case MessageID::P_Position:
			// Informs other conditions that it shouldn't request
			// another position out so soon
			position_timer = system_clock::now();
			packet << MainCharacter[player_num]->Data1.Entity->Position;
			break;

		case MessageID::P_Scale:
			packet << MainCharacter[player_num]->Data1.Entity->Scale;
			break;

		case MessageID::P_Powerups:
			PrintDebug("<< Sending powerups");
			packet << MainCharacter[player_num]->Data2.Character->Powerups;
			break;

		case MessageID::P_Upgrades:
			PrintDebug("<< Sending upgrades");
			packet << MainCharacter[player_num]->Data2.Character->Upgrades;
			break;

		case MessageID::P_Speed:
			rotate_timer = speed_timer = system_clock::now();
			packet << MainCharacter[player_num]->Data2.Character->Speed << MainCharacter[player_num]->Data2.Character->PhysData.BaseSpeed;
			break;

		case MessageID::P_Animation:
			packet << MainCharacter[player_num]->Data2.Character->AnimInfo.Next;
			break;

		case MessageID::P_SpinTimer:
			packet << reinterpret_cast<SonicCharObj2*>(MainCharacter[player_num]->Data2.Character)->SpindashCounter;
			break;

#pragma endregion

#pragma region System

		case MessageID::S_KeepAlive:
			sent_keep_alive = system_clock::now();
			break;

		case MessageID::S_2PReady:
			packet << PlayerReady[player_num];
			local.menu.PlayerReady[player_num] = PlayerReady[player_num];
			break;

		case MessageID::S_2PSpecials:
		{
			const auto& specials = player_num == 0 ? P1SpecialAttacks : P2SpecialAttacks;

			packet << specials[0] << specials[1] << specials[2];

			auto& local_specials = local.game.SpecialAttacks[player_num];

			local_specials[0] = specials[0];
			local_specials[1] = specials[1];
			local_specials[2] = specials[2];

			break;
		}

		case MessageID::S_BattleOptions:
			packet.write_data(BattleOptions, BattleOptions_Length, true);
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

			local.game.TimerMinutes = TimerMinutes;
			local.game.TimerSeconds = TimerSeconds;
			local.game.TimerFrames  = TimerFrames;
			break;

		case MessageID::S_TimeStop:
			PrintDebug("<< Sending Time Stop");
			packet << TimeStopped;
			local.game.TimeStopped = TimeStopped;
			break;

#pragma endregion
	}

	packet.finalize();
	add_type_sent(packet_type, packet.get_type_size(), packet.protocol);

	return true;
}

void PacketBroker::disconnect(node_t node)
{
	auto it = node_connections_.find(node);

	if (it == node_connections_.end())
	{
		return;
	}

	std::shared_ptr<Connection> connection = std::move(it->second);
	node_connections_.erase(it);
	connection_nodes_.erase(connection.get());
}

#pragma endregion
#pragma region Receive

// TODO: remove from this class
bool PacketBroker::receive_system(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	if (round_started())
	{
		switch (type)
		{
			default:
				return false;

			case MessageID::S_Time:
				packet >> local.game.TimerMinutes >> local.game.TimerSeconds >> local.game.TimerFrames;
				TimerMinutes = local.game.TimerMinutes;
				TimerSeconds = local.game.TimerSeconds;
				TimerFrames  = local.game.TimerFrames;
				break;

			RECEIVED(MessageID::S_GameState);
				{
					short recv_game_state;
					packet >> recv_game_state;

					if (GameState >= GameState::NormalRestart && recv_game_state > GameState::LoadFinished)
					{
						GameState = local.system.GameState = recv_game_state;
					}

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
				for (size_t i = 0; i < 3; i++)
				{
					packet >> local.game.SpecialAttacks[pnum][i];
				}

				SpecialActivateTimer[pnum] = 60;
				memcpy(pnum == 0 ? P1SpecialAttacks : P2SpecialAttacks, local.game.SpecialAttacks[pnum], sizeof(char) * 3);
				break;
		}

		return true;
	}

	return false;
}

bool PacketBroker::receive_player(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	if (round_started() && pnum != player_num)
	{
		write_player = (type > MessageID::P_START && type < MessageID::P_END);

		switch (type)
		{
			default:
				return false;

			RECEIVED(MessageID::P_Action);
				packet >> net_player[pnum].data1.Action;
				break;

			RECEIVED(MessageID::P_NextAction);
				packet >> net_player[pnum].data1.NextAction;
				break;

			RECEIVED(MessageID::P_Status);
				packet >> net_player[pnum].data1.Status;
				break;

			RECEIVED(MessageID::P_Rotation);
				packet >> net_player[pnum].data1.Rotation;
				break;

			RECEIVED(MessageID::P_Position);
				packet >> net_player[pnum].data1.Position;
				break;

			RECEIVED(MessageID::P_Scale);
				packet >> net_player[pnum].data1.Scale;
				break;

			RECEIVED(MessageID::P_Powerups);
				packet >> net_player[pnum].data2.Powerups;
				break;

			RECEIVED(MessageID::P_Upgrades);
				packet >> net_player[pnum].data2.Upgrades;
				break;

			RECEIVED(MessageID::P_HP);
				float hp, diff;
				packet >> hp >> diff;
				PrintDebug(">> HP CHANGE: %f + %f", hp, diff);

				MainCharacter[pnum]->Data2.Character->MechHP = hp;
				events::AddHP_original(pnum, diff);
				net_player[pnum].data2.MechHP = MainCharacter[pnum]->Data2.Character->MechHP;
				break;

			RECEIVED(MessageID::P_Speed);
				packet >> net_player[pnum].data2.Speed;
				packet >> net_player[pnum].data2.PhysData.BaseSpeed;
				break;

			RECEIVED(MessageID::P_Animation);
				packet >> net_player[pnum].data2.AnimInfo.Next;
				break;

			RECEIVED(MessageID::P_SpinTimer);
				packet >> net_player[pnum].sonic.SpindashCounter;
				break;
		}

		return write_player;
	}

	return false;
}

bool PacketBroker::receive_menu(MessageID type, pnum_t pnum, sws::Packet& packet)
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
				for (signed char& option : local.menu.BattleOptions)
				{
					packet >> option;
				}

				memcpy(BattleOptions, local.menu.BattleOptions, sizeof(char) * 4);

				break;

			RECEIVED(MessageID::M_BattleConfigSelection);
				packet >> local.menu.BattleOptionsSelection
					>> local.menu.BattleOptionsBack;
				BattleOptionsSelection = local.menu.BattleOptionsSelection;
				BattleOptionsBack      = local.menu.BattleOptionsBack;

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

bool PacketBroker::run_message_reader(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	const auto it = message_readers.find(type);

	if (it == message_readers.end())
	{
		return false;
	}

	return it->second(type, pnum, packet);
}

#pragma endregion
