#include <string>
#include <SFML/Network.hpp>

#include "Globals.h"			// for Globals :specialed:
#include "Networking.h"			// for MSG
#include "MemoryManagement.h"	// for MemManage
#include "CommonEnums.h"		// for Menu, SubMenu2P
#include "AddressList.h"		// for CurrentMenu

#include "Program.h"

using namespace std;
using namespace chrono;
using namespace nethax;

const char* musicConnecting		= "chao_k_m2.adx"; // originally chao_k_net_connect.adx, but that's super short and annoying
const char* musicConnected		= "chao_k_net_fine.adx";
const char* musicDisconnected	= "chao_k_net_fault.adx";
const char* musicDefault		= "btl_sel.adx";

sf::Packet& operator <<(sf::Packet& packet, const Program::Version& data)
{
	return packet << data.major << data.minor;
}
sf::Packet& operator >>(sf::Packet& packet, Program::Version& data)
{
	return packet >> data.major >> data.minor;
}

#pragma region Embedded Types

Program::Version Program::versionNum = { 3, 1 };
const std::string Program::version = "SA2:BN Version: " + Program::versionNum.str();
std::string Program::Version::str()
{
	return to_string(major) + "." + to_string(minor);
}

#pragma endregion

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
		return result;

	PlayMusic(musicDefault);
	PlayJingle(musicConnected);

	Globals::Broker->SetConnectTime();

	ApplySettings(true);
	P2Start = 2;

	return result;
}

void Program::Disconnect()
{
	setMusic = false;

	PrintDebug("<> Disconnecting...");
	Globals::Networking->Disconnect();

	ApplySettings(false);

	Globals::Broker->Initialize();
	PlayJingle(musicDisconnected);
}

void Program::ApplySettings(const bool apply)
{
	if (apply)
		PrintDebug("<> Applying code changes...");
	else
		PrintDebug("<> Reverting code changes...");

	if (clientSettings.noSpecials)
		MemManage::nop2PSpecials(apply);
	if (clientSettings.isLocal)
		MemManage::swapInput(apply);
	if (clientSettings.KeepWindowActive)
		MemManage::keepActive(apply);

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

	uint8 id;
	packet >> id;
	if (id != Message::N_VersionCheck)
	{
		PrintDebug(">> Received malformed packet from client!");
		return false;
	}

	packet >> remoteVersion;

	if (versionNum != remoteVersion)
	{
		Globals::Networking->Disconnect();
		PrintDebug("\n>> Connection rejected; the client's version does not match the local version.");
		PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());

		packet << (uint8)Message::N_VersionMismatch << versionNum;
		Globals::Networking->sendSafe(packet);
		packet.clear();

		return false;
	}

	packet >> id;

	if (id != Message::N_Bind)
	{
		PrintDebug(">> Error receiving local port from client (ID mismatch).");
		Globals::Networking->Disconnect();

		return false;
	}

	ushort remoteport;
	packet >> remoteport;
	Globals::Networking->setRemotePort(remoteport);

	packet.clear();
	packet << (uint8)Message::N_VersionOK;

	if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Status::Done)
	{
		PrintDebug(">> An error occurred while confirming version numbers with the client.");
		return false;
	}

	packet.clear();
	packet << (uint8)Message::N_Settings << clientSettings.noSpecials
		<< (uint8)Message::N_Connected;

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

	// HACK: Why is the socket accessible?
	packet << (uint8)Message::N_VersionCheck << versionNum.major << versionNum.minor
		<< (uint8)Message::N_Bind << Globals::Networking->getLocalPort();

	if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Done)
	{
		PrintDebug("<< An error occurred while sending the version number!");
		return false;
	}

	packet.clear();

	// TODO: Timeout on both of these loops.
	uint8 id = Message::None;
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

				case Message::N_VersionMismatch:
					packet >> remoteVersion;
					PrintDebug("\n>> Connection rejected; the server's version does not match the local version.");
					PrintDebug("->\tYour version: %s - Remote version: %s", versionNum.str().c_str(), remoteVersion.str().c_str());
					rejected = true;
					return false;

				case Message::N_VersionOK:
					PrintDebug(">> Version match!");
					break;

					// This is only used for specials right now.
				case Message::N_Settings:
					packet >> clientSettings.noSpecials;
					PrintDebug(">> Specials %s by server.", clientSettings.noSpecials ? "disabled" : "enabled");
					break;

				case Message::N_Connected:
					PrintDebug("<< Connected!");
					break;
			}
		}
	} while (id != Message::N_Connected);

	return true;
}
