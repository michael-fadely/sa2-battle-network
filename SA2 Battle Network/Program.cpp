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

string Program::Version::str() const
{
	return to_string(major) + "." + to_string(minor);
}

#pragma endregion

const char* musicConnecting		= "chao_k_m2.adx"; // originally chao_k_net_connect.adx, but that's super short and annoying
const char* musicConnected		= "chao_k_net_fine.adx";
const char* musicDisconnected	= "chao_k_net_fault.adx";
const char* musicDefault		= "btl_sel.adx";

const Program::Version	Program::versionNum	= { 3, 2 };
const string		Program::version	= "SA2:BN Version: " + versionNum.str();

Program::Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address) :
	remoteVersion(versionNum), localSettings(settings), remoteSettings({}), Address(address), isServer(host), setMusic(false), rejected(false)
{
}

bool Program::CheckConnectOK()
{
	return CurrentMenu[0] >= Menu::BATTLE &&
		(CurrentMenu[1] > SubMenu2P::I_START || Globals::isConnected());
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

	ConnectStatus result = (isServer) ? startServer() : startClient();

	if (result != ConnectStatus::Success)
	{
		if (result == ConnectStatus::Error)
			Globals::Networking->Disconnect();
	}
	else
	{
		PlayMusic(musicDefault);
		PlayJingle(musicConnected);

		Globals::Broker->ToggleNetStat(localSettings.netStat);
		Globals::Broker->SetConnectTime();

		applySettings(true);
		P2Start = 2;
		FrameCount = 0;
	}

	return result == ConnectStatus::Success;
}

void Program::Disconnect()
{
	setMusic = false;

	PrintDebug("<> Disconnecting...");
	Globals::Networking->Disconnect();

	applySettings(false);

	if (!isServer)
		ApplySettings(localSettings);

	Globals::Broker->SaveNetStat();
	Globals::Broker->Initialize();
	PlayJingle(musicDisconnected);
}

void Program::ApplySettings(const Settings& settings)
{
	MemManage::nop2PSpecials(settings.noSpecials);
	MemManage::swapInput(settings.local);
	CheatsEnabled = (Bool)settings.cheats;
	if (!CheatsEnabled)
	{
		Cheats_GiveMaxRings		= 0;
		Cheats_GiveAllUpgrades	= 0;
		Cheats_GiveMaxLives		= 0;
		Cheats_ExitStage		= 0;
	}
}

void Program::applySettings(bool apply) const
{
	if (!isServer)
		ApplySettings(remoteSettings);
			
	if (isServer)
		apply = !apply;

	MemManage::swapSpawn(apply);
	MemManage::swapCharsel(apply);
}

Program::ConnectStatus Program::startServer()
{
	sf::Packet packet;
	sf::Socket::Status status = sf::Socket::Status::Error;

	if (!Globals::Networking->isBound())
		PrintDebug("Hosting server on port %d...", Address.port);

	if ((status = Globals::Networking->Listen(Address.port, false)) != sf::Socket::Done)
	{
		if (status == sf::Socket::Error)
			PrintDebug("<> An error occurred while trying to listen for connections on port %d", Address.port);

		return ConnectStatus::Listening;
	}

	if ((status = Globals::Networking->ReceiveSafe(packet, true)) != sf::Socket::Done)
	{
		PrintDebug(">> An error occurred while waiting for version number.");
		return ConnectStatus::Error;
	}

	bool passwordProtected = !localSettings.password.empty();
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
				return ConnectStatus::Error;

			case MessageID::N_VersionCheck:
			{
				packet >> remoteVersion;

				if (versionNum != remoteVersion)
				{
					PrintDebug("\n>> Connection rejected; the client's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());

					packet << MessageID::N_VersionMismatch << versionNum;
					Globals::Networking->SendSafe(packet);

					return ConnectStatus::Error;
				}

				break;
			}

			case MessageID::N_Password:
			{
				vector<uchar> password;
				packet >> password;

				if (!passwordProtected)
					break;

				if (password.size() != localSettings.password.size())
				{
					PrintDebug(">> Hash length discrepency.");
				}
				else
				{
					passwordMatch = !memcmp(localSettings.password.data(), password.data(), password.size());

					if (!passwordMatch)
						PrintDebug(">> Client sent invalid password.");
				}

				break;
			}

			case MessageID::N_Bind:
			{
				packet >> remotePort;
				Globals::Networking->SetRemotePort(remotePort);
				break;
			}
		}
	}

	if (passwordProtected && !passwordMatch)
	{
		packet.clear();
		packet << MessageID::N_PasswordMismatch;
		Globals::Networking->SendSafe(packet);
		return ConnectStatus::Error;
	}

	if (!remotePort)
	{
		PrintDebug(">> Error: Client didn't send their port!");
		return ConnectStatus::Error;
	}

	packet.clear();
	packet << MessageID::N_VersionOK;

	if ((status = Globals::Networking->SendSafe(packet)) != sf::Socket::Status::Done)
	{
		PrintDebug(">> An error occurred while confirming version numbers with the client.");
		return ConnectStatus::Error;
	}

	packet.clear();
	packet << MessageID::N_Settings << localSettings << MessageID::N_Connected;

	if ((status = Globals::Networking->SendSafe(packet)) != sf::Socket::Status::Done)
	{
		PrintDebug(">> An error occurred while confirming the connection with the client.");
		return ConnectStatus::Error;
	}

	PrintDebug(">> Connected!");
	return ConnectStatus::Success;
}

Program::ConnectStatus Program::startClient()
{
	sf::Packet packet;
	sf::Socket::Status status = sf::Socket::Status::Error;

	if (!Globals::Networking->isBound())
		PrintDebug("<< Connecting to server at %s on port %d...", Address.ip.toString().c_str(), Address.port);

	if ((status = Globals::Networking->Connect(Address, false)) != sf::Socket::Done)
	{
		if (status == sf::Socket::Error)
			PrintDebug("<< A connection error has occurred.");

		return ConnectStatus::Listening;
	}

	packet << MessageID::N_VersionCheck << versionNum.major << versionNum.minor;

	if (!localSettings.password.empty())
	{
		packet << MessageID::N_Password << localSettings.password;
	}

	packet << MessageID::N_Bind << Globals::Networking->GetLocalPort();

	if ((status = Globals::Networking->SendSafe(packet)) != sf::Socket::Done)
	{
		PrintDebug("<< An error occurred while sending the version number!");
		return ConnectStatus::Error;
	}

	packet.clear();

	// TODO: Timeout on both of these loops.
	MessageID id = MessageID::None;
	do
	{
		if ((status = Globals::Networking->ReceiveSafe(packet, true)) != sf::Socket::Done)
		{
			PrintDebug(">> An error occurred while confirming the connection with the server.");
			return ConnectStatus::Error;
		}

		while (!packet.endOfPacket())
		{
			packet >> id;

			switch (id)
			{
				default:
					PrintDebug(">> Received malformed packet from server; aborting!");
					rejected = true;
					return ConnectStatus::Error;

				case MessageID::N_VersionMismatch:
					packet >> remoteVersion;
					PrintDebug("\n>> Connection rejected; the server's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());
					rejected = true;
					return ConnectStatus::Error;

				case MessageID::N_VersionOK:
					PrintDebug(">> Version match!");
					break;

					// This is only used for specials right now.
				case MessageID::N_Settings:
					packet >> remoteSettings;
					PrintDebug(">> Specials %s by server.", remoteSettings.noSpecials ? "disabled" : "enabled");
					if (remoteSettings.cheats)
						PrintDebug(">> Cheats have been enabled by the server!");
					break;

				case MessageID::N_PasswordMismatch:
					PrintDebug(!localSettings.password.empty() ? ">> Invalid password." : ">> This server is password protected.");
					rejected = true;
					return ConnectStatus::Error;

				case MessageID::N_Connected:
					PrintDebug("<< Connected!");
					break;
			}
		}
	} while (id != MessageID::N_Connected);

	return ConnectStatus::Success;
}
