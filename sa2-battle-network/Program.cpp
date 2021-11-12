#include "stdafx.h"

#include <string>

#include <sws/SocketError.h>

#include "globals.h"
#include "Networking.h"
#include "MemoryManagement.h" // for MemManage
#include "CommonEnums.h"      // for Menu, SubMenu2P
#include "AddressList.h"      // for CurrentMenu
#include "PacketOverloads.h"
#include "Events.h"

#include "Program.h"
#include "ChangeMusic.h"
#include "ConnectionManager.h"

using namespace nethax;

#pragma region Embedded Types

static sws::Packet& operator<<(sws::Packet& packet, const Program::Version& data)
{
	return packet << data.major << data.minor;
}

static sws::Packet& operator>>(sws::Packet& packet, Program::Version& data)
{
	return packet >> data.major >> data.minor;
}

static sws::Packet& operator<<(sws::Packet& packet, const Program::Settings& data)
{
	return packet << data.no_specials << data.cheats;
}

static sws::Packet& operator>>(sws::Packet& packet, Program::Settings& data)
{
	return packet >> data.no_specials >> data.cheats;
}

std::string Program::Version::str() const
{
	return std::to_string(major) + "." + std::to_string(minor);
}

bool Program::Version::operator==(const Version& value) const
{
	return major == value.major && minor == value.minor;
}

bool Program::Version::operator!=(const Version& value) const
{
	return !(*this == value);
}

#pragma endregion

static const char* music_connecting   = "chao_k_m2.adx";
static const char* music_connected    = "chao_k_net_fine.adx";
static const char* music_disconnected = "chao_k_net_fault.adx";
static const char* music_default      = "btl_sel.adx";

const Program::Version Program::version_num = { 3, 2 };
const std::string Program::version_string = "SA2:BN Version: " + version_num.str();

Program::Program(const Settings& settings, const bool is_server, const sws::Address& address)
	: remote_version_(version_num),
	  local_settings_(settings),
	  remote_settings_({}),
	  server_address_(address),
	  is_server_(is_server),
	  set_music_(false),
	  rejected_(false),
	  player_num_(-1)
{
}

bool Program::can_connect()
{
	return CurrentMenu >= Menu::battle &&
	       (CurrentSubMenu > SubMenu2P::i_start || globals::is_connected());
}

bool Program::connect()
{
	if (!can_connect())
	{
		set_music_ = false;
		rejected_ = false;
		return false;
	}

	if (!is_server_ && rejected_)
	{
		return false;
	}

	if (!set_music_)
	{
		ChangeMusic(music_connecting);
		set_music_ = true;
	}

	const ConnectStatus result = is_server_ ? start_server() : start_client();

	if (result != ConnectStatus::success)
	{
		if (result == ConnectStatus::error)
		{
			globals::broker->disconnect();
		}
	}
	else
	{
		PlayMusic(music_default);
		PlayJingle(music_connected);

		globals::broker->toggle_netstat(local_settings_.netstat);

		if (player_num_ >= 0)
		{
			globals::broker->set_player_number(player_num_);
		}

		apply_settings();
		P2Start = 2;

		if (globals::broker->connection_count())
		{
			events::Initialize();
		}
	}

	return result == ConnectStatus::success;
}

void Program::disconnect()
{
	set_music_ = false;

	PrintDebug("<> Disconnecting...");
	globals::broker->disconnect();

	apply_settings();
	events::Deinitialize();

	if (!is_server_)
	{
		apply_settings(local_settings_);
	}

	globals::broker->save_netstat();
	globals::broker->initialize();
	player_num_ = -1;
	PlayJingle(music_disconnected);
}

void Program::apply_settings(const Settings& settings)
{
	mem_manage::nop_specials(settings.no_specials);

	CheatsEnabled = static_cast<Bool>(settings.cheats);

	if (!CheatsEnabled)
	{
		Cheats_GiveMaxRings    = 0;
		Cheats_GiveAllUpgrades = 0;
		Cheats_GiveAllEmblems  = 0;
		Cheats_ExitStage       = 0;
	}
}

void Program::apply_settings() const
{
	if (!is_server_)
	{
		apply_settings(remote_settings_);
	}
}

// TODO: player count limit
// TODO: play type (normal, spectate)
Program::ConnectStatus Program::start_server()
{
	sws::Packet packet;
	sws::SocketState status;

	const std::shared_ptr<ConnectionManager> manager = globals::broker->connection_manager();
	std::shared_ptr<Connection> connection;

	if (!manager->is_bound())
	{
		PrintDebug("Hosting server on port %d...", server_address_.port);

		if ((status = manager->host(server_address_)) != sws::SocketState::done)
		{
			PrintDebug("Host failed!");
			return ConnectStatus::error;
		}

		is_server_ = true;
	}

	if ((status = manager->listen(&connection)) != sws::SocketState::done)
	{
		if (status == sws::SocketState::error)
		{
			PrintDebug("<> An error occurred while trying to listen for connections on port %d", server_address_.port);
		}

		return ConnectStatus::listening;
	}

	if ((status = manager->receive(true)) != sws::SocketState::done)
	{
		manager->disconnect(connection);
		PrintDebug(">> An error occurred while waiting for version number.");
		return ConnectStatus::error;
	}

	if (!connection->pop(&packet))
	{
		manager->disconnect(connection);
		return ConnectStatus::error;
	}

	const bool has_password = !local_settings_.password.empty();
	bool match = false;

	MessageID id = MessageID::None;
	while (!packet.end())
	{
		packet >> id;

		switch (id)
		{
			default:
				PrintDebug(">> Received malformed packet from client!");
				manager->disconnect(connection);
				return ConnectStatus::error;

			case MessageID::N_VersionCheck:
			{
				packet >> remote_version_;

				if (version_num != remote_version_)
				{
					PrintDebug("\n>> Connection rejected; the client's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", version_num.str().c_str(), remote_version_.str().c_str());

					packet.clear();
					reliable::reserve(packet, reliable::reliable_t::ack_any);
					packet << MessageID::N_VersionMismatch << version_num;
					connection->send(packet, true);

					manager->disconnect(connection);
					return ConnectStatus::error;
				}

				break;
			}

			case MessageID::N_Password:
			{
				std::vector<uint8_t> password;
				packet >> password;

				if (!has_password)
				{
					break;
				}

				if (password.size() != local_settings_.password.size())
				{
					PrintDebug(">> Hash length discrepancy.");
				}
				else
				{
					match = !memcmp(local_settings_.password.data(), password.data(), password.size());

					if (!match)
					{
						PrintDebug(">> Client sent invalid password.");
					}
				}

				break;
			}
		}
	}

	if (has_password && !match)
	{
		packet.clear();
		reliable::reserve(packet, reliable::reliable_t::ack_any);
		packet << MessageID::N_PasswordMismatch;
		connection->send(packet, true);
		manager->disconnect(connection);
		return ConnectStatus::error;
	}

	packet.clear();
	reliable::reserve(packet, reliable::reliable_t::ack_any);
	packet
		<< MessageID::N_VersionOK
		<< MessageID::N_Settings << local_settings_
		<< MessageID::N_SetPlayerNumber << static_cast<pnum_t>(manager->connection_count())
		<< MessageID::N_Connected;

	if ((status = connection->send(packet, true)) != sws::SocketState::done)
	{
		PrintDebug(">> An error occurred while confirming the connection with the client.");
		manager->disconnect(connection);
		return ConnectStatus::error;
	}

	player_num_ = 0;
	globals::broker->add_client(std::move(connection));

	PrintDebug(">> Connected!");
	return ConnectStatus::success;
}

Program::ConnectStatus Program::start_client()
{
	sws::SocketState status;
	std::shared_ptr<Connection> connection;
	const std::shared_ptr<ConnectionManager> manager = globals::broker->connection_manager();

	if (!manager->is_bound())
	{
		PrintDebug("<< Connecting to server at %s on port %d...", server_address_.address.c_str(), server_address_.port);
	}

	if ((status = manager->connect(server_address_, &connection)) != sws::SocketState::done)
	{
		if (status == sws::SocketState::error)
		{
			PrintDebug("<< A connection error has occurred.");
			return ConnectStatus::error;
		}

		return ConnectStatus::listening;
	}

	sws::Packet packet = reliable::reserve(reliable::reliable_t::ack_any);
	packet << MessageID::N_VersionCheck << version_num;

	if (!local_settings_.password.empty())
	{
		packet << MessageID::N_Password << local_settings_.password;
	}

	if ((status = connection->send(packet, true)) != sws::SocketState::done)
	{
		PrintDebug("<< Error sending request to server.");
		return ConnectStatus::error;
	}

	packet.clear();

	// TODO: Timeout on both of these loops.
	MessageID id = MessageID::None;

	do
	{
		if ((status = manager->receive(true)) != sws::SocketState::done)
		{
			PrintDebug(">> An error occurred while confirming the connection with the server.");
			manager->disconnect(connection);
			return ConnectStatus::error;
		}

		while (connection->pop(&packet))
		{
			while (!packet.end())
			{
				packet >> id;

				switch (id)
				{
					default:
						PrintDebug(">> Received malformed packet from server; aborting!");
						rejected_ = true;
						manager->disconnect(connection);
						return ConnectStatus::error;

					case MessageID::N_VersionMismatch:
						packet >> remote_version_;
						PrintDebug("\n>> Connection rejected; the server's version does not match the local version.");
						PrintDebug("->\tYour version: %s - Remote version: %s", version_num.str().c_str(), remote_version_.str().c_str());
						rejected_ = true;
						manager->disconnect(connection);
						return ConnectStatus::error;

					case MessageID::N_VersionOK:
						PrintDebug(">> Version match!");
						break;

						// This is only used for specials right now.
					case MessageID::N_Settings:
						packet >> remote_settings_;
						PrintDebug(">> Specials %s by server.", remote_settings_.no_specials ? "disabled" : "enabled");

						if (remote_settings_.cheats)
						{
							PrintDebug(">> Cheats have been enabled by the server!");
						}
						break;

					case MessageID::N_PasswordMismatch:
						PrintDebug(!local_settings_.password.empty() ? ">> Invalid password." : ">> This server is password protected.");
						rejected_ = true;
						manager->disconnect(connection);
						return ConnectStatus::error;

					case MessageID::N_SetPlayerNumber:
						packet >> player_num_;
						PrintDebug("Received player number: %d", player_num_);
						break;

					case MessageID::N_Connected:
						PrintDebug("<< Connected!");
						break;
				}
			}
		}
	} while (id != MessageID::N_Connected);

	globals::broker->add_server(std::move(connection));

	return ConnectStatus::success;
}
