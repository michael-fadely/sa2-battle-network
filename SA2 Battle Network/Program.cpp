#include "stdafx.h"

#include <string>
#include <SFML/Network.hpp>

#include "Globals.h"			// for Globals :specialed:
#include "Networking.h"
#include "MemoryManagement.h"	// for MemManage
#include "CommonEnums.h"		// for Menu, SubMenu2P
#include "AddressList.h"		// for CurrentMenu
#include "PacketOverloads.h"

#include "Program.h"

using namespace std;
using namespace nethax;

#pragma region Embedded Types

sf::Packet& operator <<(sf::Packet& packet, const Program::Version& data)
{
	return packet << data.major << data.minor;
}
sf::Packet& operator >>(sf::Packet& packet, Program::Version& data)
{
	return packet >> data.major >> data.minor;
}

sf::Packet& operator<<(sf::Packet& packet, const Program::Settings& data)
{
	return packet << data.noSpecials << data.cheats;
}
sf::Packet& operator >>(sf::Packet& packet, Program::Settings& data)
{
	return packet >> data.noSpecials >> data.cheats;
}

std::string Program::Version::str() const
{
	return to_string(major) + "." + to_string(minor);
}

#pragma endregion

const char* musicConnecting		= "chao_k_m2.adx"; // originally chao_k_net_connect.adx, but that's super short and annoying
const char* musicConnected		= "chao_k_net_fine.adx";
const char* musicDisconnected	= "chao_k_net_fault.adx";
const char* musicDefault		= "btl_sel.adx";

const Program::Version	Program::versionNum	= { 3, 2 };
const std::string		Program::version	= "SA2:BN Version: " + Program::versionNum.str();

Program::Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address) :
	remoteVersion(Program::versionNum), clientSettings(settings), Address(address), isServer(host), setMusic(false), rejected(false)
{
}

bool Program::CheckConnectOK() const
{
	return CurrentMenu[0] >= Menu::BATTLE && CurrentMenu[1] > SubMenu2P::I_START;
}

bool Program::Connect()
{
	if (!CheckConnectOK())
	{
		setMusic = false;
		rejected = false;
		return false;
	}

	if (!isServer && rejected)
		return false;

	if (!setMusic)
	{
		ChangeMusic(musicConnecting);
		setMusic = true;
	}

	bool result = (isServer) ? StartServer() : StartClient();

	if (!result)
	{
		Globals::Networking->Disconnect();
		return result;
	}

	PlayMusic(musicDefault);
	PlayJingle(musicConnected);

	Globals::Broker->ToggleNetStat(clientSettings.netStat);
	Globals::Broker->SetConnectTime();

	ApplySettings(true);
	P2Start = 2;
	FrameCount = 0;

	return result;
}

void Program::Disconnect()
{
	setMusic = false;

	PrintDebug("<> Disconnecting...");
	Globals::Networking->Disconnect();

	ApplySettings(false);

	Globals::Broker->SaveNetStat();
	Globals::Broker->Initialize();
	PlayJingle(musicDisconnected);
}

void Program::ApplySettings(const bool apply)
{
	PrintDebug(apply ? "<> Applying code changes..." : "<> Reverting code changes...");

	if (clientSettings.noSpecials)
		MemManage::nop2PSpecials(apply);
	if (clientSettings.local)
		MemManage::swapInput(apply);

	MemManage::swapSpawn(isServer ? !apply : apply);
	MemManage::swapCharsel(isServer ? !apply : apply);
}

bool Program::StartServer()
{
	sf::Packet packet;
	sf::Socket::Status status = sf::Socket::Status::Error;

	if (!Globals::Networking->isBound())
		PrintDebug("Hosting server on port %d...", Address.port);

	if ((status = Globals::Networking->Listen(Address.port, false)) != sf::Socket::Done)
	{
		if (status == sf::Socket::Error)
			PrintDebug("<> An error occurred while trying to listen for connections on port %d", Address.port);

		return false;
	}

	if (!CheckConnectOK())
		return false;

	if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
	{
		PrintDebug(">> An error occurred while waiting for version number.");
		return false;
	}

	bool passwordProtected = !clientSettings.password.empty();
	bool passwordMatch = false;
	ushort remotePort = 0;

	MessageID id = MessageID::None;
	while (!packet.endOfPacket())
	{
		packet >> id;

		switch (id)
		{
			default:
				PrintDebug(">> Received malformed packet from client!");
				return false;

			case MessageID::N_VersionCheck:
			{
				packet >> remoteVersion;

				if (versionNum != remoteVersion)
				{
					PrintDebug("\n>> Connection rejected; the client's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());

					packet << MessageID::N_VersionMismatch << versionNum;
					Globals::Networking->sendSafe(packet);

					return false;
				}

				break;
			}

			case MessageID::N_Password:
			{
				std::vector<char> password;
				packet >> password;

				if (!passwordProtected)
					break;

				if (password.size() != clientSettings.password.size())
				{
					PrintDebug(">> Hash length discrepency.");
				}
				else
				{
					passwordMatch = !memcmp(clientSettings.password.data(), password.data(), password.size());

					if (!passwordMatch)
						PrintDebug(">> Client sent invalid password.");
				}

				break;
			}

			case MessageID::N_Bind:
			{
				packet >> remotePort;
				Globals::Networking->setRemotePort(remotePort);
				break;
			}
		}
	}

	if (passwordProtected && !passwordMatch)
	{
		packet.clear();
		packet << MessageID::N_PasswordMismatch;
		Globals::Networking->sendSafe(packet);
		return false;
	}

	if (!remotePort)
	{
		PrintDebug(">> Error: Client didn't send their port!");
		return false;
	}

	packet.clear();
	packet << MessageID::N_VersionOK;

	if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Status::Done)
	{
		PrintDebug(">> An error occurred while confirming version numbers with the client.");
		return false;
	}

	packet.clear();
	packet << MessageID::N_Settings << clientSettings << MessageID::N_Connected;

	if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Status::Done)
	{
		PrintDebug(">> An error occurred while confirming the connection with the client.");
		return false;
	}

	PrintDebug(">> Connected!");
	return true;
}

bool Program::StartClient()
{
	sf::Packet packet;
	sf::Socket::Status status = sf::Socket::Status::Error;

	if (!Globals::Networking->isBound())
		PrintDebug("<< Connecting to server at %s on port %d...", Address.ip.toString().c_str(), Address.port);

	if ((status = Globals::Networking->Connect(Address, false)) != sf::Socket::Done)
	{
		if (status == sf::Socket::Error)
			PrintDebug("<< A connection error has occurred.");

		return false;
	}

	if (!CheckConnectOK())
		return false;

	packet << MessageID::N_VersionCheck << versionNum.major << versionNum.minor;

	if (!clientSettings.password.empty())
	{
		packet << MessageID::N_Password << clientSettings.password;
	}

	packet << MessageID::N_Bind << Globals::Networking->getLocalPort();

	if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Done)
	{
		PrintDebug("<< An error occurred while sending the version number!");
		return false;
	}

	packet.clear();

	// TODO: Timeout on both of these loops.
	MessageID id = MessageID::None;
	do
	{
		if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
		{
			PrintDebug(">> An error occurred while confirming the connection with the server.");
			return false;
		}

		while (!packet.endOfPacket())
		{
			packet >> id;

			switch (id)
			{
				default:
					PrintDebug(">> Received malformed packet from server; aborting!");
					rejected = true;
					return false;

				case MessageID::N_VersionMismatch:
					packet >> remoteVersion;
					PrintDebug("\n>> Connection rejected; the server's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());
					rejected = true;
					return false;

				case MessageID::N_VersionOK:
					PrintDebug(">> Version match!");
					break;

					// This is only used for specials right now.
				case MessageID::N_Settings:
					packet >> clientSettings;
					PrintDebug(">> Specials %s by server.", clientSettings.noSpecials ? "disabled" : "enabled");
					if (clientSettings.cheats)
						PrintDebug(">> Cheats have been enabled by the server!");
					break;

				case MessageID::N_PasswordMismatch:
					PrintDebug(!clientSettings.password.empty() ? ">> Invalid password." : ">> This server is password protected.");
					rejected = true;
					return false;

				case MessageID::N_Connected:
					PrintDebug("<< Connected!");
					break;
			}
		}
	} while (id != MessageID::N_Connected);

	return true;
}
