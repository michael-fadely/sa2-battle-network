#include "stdafx.h"

#include <string>

#include <sws/SocketError.h>

#include "globals.h"			// for Globals :specialed:
#include "Networking.h"
#include "MemoryManagement.h"	// for MemManage
#include "CommonEnums.h"		// for Menu, SubMenu2P
#include "AddressList.h"		// for CurrentMenu
#include "PacketOverloads.h"
#include "Events.h"

#include "Program.h"

using namespace std;
using namespace nethax;

#pragma region Embedded Types

sws::Packet& operator <<(sws::Packet& packet, const Program::Version& data)
{
	return packet << data.major << data.minor;
}
sws::Packet& operator >>(sws::Packet& packet, Program::Version& data)
{
	return packet >> data.major >> data.minor;
}

sws::Packet& operator<<(sws::Packet& packet, const Program::Settings& data)
{
	return packet << data.no_specials << data.cheats;
}
sws::Packet& operator >>(sws::Packet& packet, Program::Settings& data)
{
	return packet >> data.no_specials >> data.cheats;
}

string Program::Version::str() const
{
	return to_string(major) + "." + to_string(minor);
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

const char* music_connecting   = "chao_k_m2.adx";
const char* music_connected    = "chao_k_net_fine.adx";
const char* music_disconnected = "chao_k_net_fault.adx";
const char* music_default      = "btl_sel.adx";

const Program::Version Program::version_num = { 3, 2 };
const string Program::version = "SA2:BN Version: " + version_num.str();

Program::Program(const Settings& settings, const bool host, const sws::Address& address) :
	remote_version(version_num), local_settings(settings), remote_settings({}), address(address), is_server(host), set_music(false), rejected(false), player_num(-1)
{
}

bool Program::can_connect()
{
	return CurrentMenu[0] >= Menu::battle &&
		(CurrentMenu[1] > SubMenu2P::i_start || globals::is_connected());
}

bool Program::connect()
{
	if (!can_connect())
	{
		set_music = false;
		rejected = false;
		return false;
	}

	if (!is_server && rejected)
	{
		return false;
	}

	if (!set_music)
	{
		ChangeMusic(music_connecting);
		set_music = true;
	}

	ConnectStatus result = (is_server) ? start_server() : start_client();

	if (result != ConnectStatus::success)
	{
		if (result == ConnectStatus::error)
		{
			globals::networking->disconnect();
		}
	}
	else
	{
		PlayMusic(music_default);
		PlayJingle(music_connected);

		globals::broker->toggle_netstat(local_settings.netstat);
		globals::broker->set_connect_time();

		if (player_num >= 0)
		{
			globals::broker->set_player_number(player_num);
		}

		apply_settings();
		P2Start = 2;

		if (globals::networking->connection_count() == 1)
		{
			events::Initialize();
		}
	}

	return result == ConnectStatus::success;
}

void Program::disconnect()
{
	set_music = false;

	PrintDebug("<> Disconnecting...");
	globals::networking->disconnect();

	apply_settings();
	events::Deinitialize();

	if (!is_server)
	{
		apply_settings(local_settings);
	}

	globals::broker->save_netstat();
	globals::broker->initialize();
	player_num = -1;
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
		Cheats_GiveMaxLives    = 0;
		Cheats_ExitStage       = 0;
	}
}

void Program::apply_settings() const
{
	if (!is_server)
	{
		apply_settings(remote_settings);
	}
}

// TODO: player count limit
// TODO: play type (normal, spectate)
Program::ConnectStatus Program::start_server()
{
	sws::Packet packet;
	sws::SocketState status;

	if (!globals::networking->is_bound())
	{
		PrintDebug("Hosting server on port %d...", address.port);
	}

	node_t node;

	if ((status = globals::networking->listen(address, node, false)) != sws::SocketState::done)
	{
		if (status == sws::SocketState::error)
		{
			PrintDebug("<> An error occurred while trying to listen for connections on port %d", address.port);
		}

		return ConnectStatus::listening;
	}

	const auto connection = globals::networking->connections().back();

	if ((status = globals::networking->receive_tcp(packet, connection, true)) != sws::SocketState::done)
	{
		PrintDebug(">> An error occurred while waiting for version number.");
		return ConnectStatus::error;
	}

	bool has_password = !local_settings.password.empty();
	bool match = false;
	ushort remote_port = 0;

	MessageID id = MessageID::None;
	while (!packet.end())
	{
		packet >> id;

		switch (id)
		{
			default:
				PrintDebug(">> Received malformed packet from client!");
				return ConnectStatus::error;

			case MessageID::N_VersionCheck:
			{
				packet >> remote_version;

				if (version_num != remote_version)
				{
					PrintDebug("\n>> Connection rejected; the client's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", version_num.str().c_str(), remote_version.str().c_str());

					packet << MessageID::N_VersionMismatch << version_num;
					globals::networking->send_tcp(packet, node);

					return ConnectStatus::error;
				}

				break;
			}

			case MessageID::N_Password:
			{
				vector<uint8_t> password;
				packet >> password;

				if (!has_password)
				{
					break;
				}

				if (password.size() != local_settings.password.size())
				{
					PrintDebug(">> Hash length discrepency.");
				}
				else
				{
					match = !memcmp(local_settings.password.data(), password.data(), password.size());

					if (!match)
					{
						PrintDebug(">> Client sent invalid password.");
					}
				}

				break;
			}

			case MessageID::N_Bind:
			{
				packet >> remote_port;
				globals::networking->set_remote_port(node, remote_port);
				break;
			}
		}
	}

	if (has_password && !match)
	{
		packet.clear();
		packet << MessageID::N_PasswordMismatch;
		globals::networking->send_tcp(packet, node);
		return ConnectStatus::error;
	}

	if (!remote_port)
	{
		PrintDebug(">> Error: Client didn't send their port!");
		return ConnectStatus::error;
	}

	packet.clear();
	packet << MessageID::N_VersionOK;

	if ((status = globals::networking->send_tcp(packet, node)) != sws::SocketState::done)
	{
		PrintDebug(">> An error occurred while confirming version numbers with the client.");
		return ConnectStatus::error;
	}

	packet.clear();
	packet << MessageID::N_Settings << local_settings
		<< MessageID::N_SetPlayer << static_cast<pnum_t>(globals::networking->connection_count())
		<< MessageID::N_Connected;

	if ((status = globals::networking->send_tcp(packet, node)) != sws::SocketState::done)
	{
		PrintDebug(">> An error occurred while confirming the connection with the client.");
		return ConnectStatus::error;
	}

	globals::broker->set_player_number(0);

	PrintDebug(">> Connected!");
	return ConnectStatus::success;
}

Program::ConnectStatus Program::start_client()
{
	sws::Packet packet;
	sws::SocketState status;

	if (!globals::networking->is_bound())
	{
		PrintDebug("<< Connecting to server at %s on port %d...", address.address.c_str(), address.port);
	}

	if ((status = globals::networking->connect(address, false)) != sws::SocketState::done)
	{
		if (status == sws::SocketState::error)
		{
			PrintDebug("<< A connection error has occurred.");
		}

		return ConnectStatus::listening;
	}

	packet << MessageID::N_VersionCheck << version_num.major << version_num.minor;

	if (!local_settings.password.empty())
	{
		packet << MessageID::N_Password << local_settings.password;
	}

	packet << MessageID::N_Bind << globals::networking->get_local_port();

	if ((status = globals::networking->send_tcp(packet)) != sws::SocketState::done)
	{
		PrintDebug("<< An error occurred while sending the version number!");
		return ConnectStatus::error;
	}

	packet.clear();

	// TODO: Timeout on both of these loops.
	MessageID id = MessageID::None;
	const auto& connection = globals::networking->connections().back();
	do
	{
		if ((status = globals::networking->receive_tcp(packet, connection, true)) != sws::SocketState::done)
		{
			PrintDebug(">> An error occurred while confirming the connection with the server.");
			return ConnectStatus::error;
		}

		while (!packet.end())
		{
			packet >> id;

			switch (id)
			{
				default:
					PrintDebug(">> Received malformed packet from server; aborting!");
					rejected = true;
					return ConnectStatus::error;

				case MessageID::N_VersionMismatch:
					packet >> remote_version;
					PrintDebug("\n>> Connection rejected; the server's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", version_num.str().c_str(), remote_version.str().c_str());
					rejected = true;
					return ConnectStatus::error;

				case MessageID::N_VersionOK:
					PrintDebug(">> Version match!");
					break;

					// This is only used for specials right now.
				case MessageID::N_Settings:
					packet >> remote_settings;
					PrintDebug(">> Specials %s by server.", remote_settings.no_specials ? "disabled" : "enabled");

					if (remote_settings.cheats)
					{
						PrintDebug(">> Cheats have been enabled by the server!");
					}
					break;

				case MessageID::N_PasswordMismatch:
					PrintDebug(!local_settings.password.empty() ? ">> Invalid password." : ">> This server is password protected.");
					rejected = true;
					return ConnectStatus::error;

				case MessageID::N_SetPlayer:
					packet >> player_num;
					PrintDebug("Received player number: %d", player_num);
					break;

				case MessageID::N_Connected:
					PrintDebug("<< Connected!");
					break;
			}
		}
	} while (id != MessageID::N_Connected);

	return ConnectStatus::success;
}
