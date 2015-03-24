#include <iostream>
#include <string>
#include <chrono>

#include <SFML/Network.hpp>

#include "Common.h"
#include "Networking.h"

#include "PacketHandler.h"
#include "MemoryManagement.h"
#include "MemoryHandler.h"
#include "ModLoaderExtensions.h"
#include "CommonEnums.h"

#include "Program.h"

// TODO: Replace music filename strings with constants or read them from a file.

using namespace std;
using namespace chrono;
using namespace sa2bn;

Program::Version Program::versionNum = { 3, 1 };
const string Program::version = "SA2:BN Version: " + Program::versionNum.str();

const std::string Program::Version::str()
{
	return to_string(major) + "." + to_string(minor);
}


Program::Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address) :
errorCode(ErrorCode::None), clientSettings(settings), remoteVersion(Program::versionNum), isServer(host), Address(address), setMusic(false)
{
}

bool Program::CheckConnectOK()
{
	return CurrentMenu[0] >= Menu::BATTLE/* && (
		(!Globals::Networking->isConnected() && CurrentMenu[1] >= SubMenu2P::S_START) ||
		(Globals::Networking->isConnected() && CurrentMenu[1] < SubMenu2P::S_START && CurrentMenu[1] >= SubMenu2P::S_BATTLEMODE))*/;
}

Program::ErrorCode Program::Connect()
{
	if (CheckConnectOK())
	{
		// Used only for connection loops.
		sf::Packet packet;
		sf::Socket::Status status = sf::Socket::Status::Error;
		bool connected = false;

		if (!setMusic)
		{
			ChangeMusic("chao_k_net_connect.adx");
			setMusic = true;
		}

		if (isServer)
		{
#pragma region Server
			if (!Globals::Networking->isBound())
				cout << "<> Hosting server on port " << Address.port << "..." << endl;

			if ((status = Globals::Networking->Listen(Address.port, false)) != sf::Socket::Done)
			{
				if (status == sf::Socket::Error)
				{
					cout << "<> An error occurred while trying to listen for connections on port " << Address.port << endl;
					return errorCode = ErrorCode::ClientTimeout;
				}
				else if (status == sf::Socket::NotReady)
				{
					return ErrorCode::NotReady;
				}
			}

			if (!connected && CheckConnectOK())
			{
				if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
				{
					cout << ">> An error occurred while waiting for version number." << endl;
					return ErrorCode::NotReady;
				}

				uint8 id;
				packet >> id;
				if (id != MSG_VERSION_CHECK)
				{
					cout << ">> Received malformed packet from client!" << endl;
					return ErrorCode::NotReady;
				}

				packet >> remoteVersion.major >> remoteVersion.minor;
				packet.clear();

				if (memcmp(&versionNum, &remoteVersion, sizeof(Version)) != 0)
				{
					Globals::Networking->Disconnect();
					cout << "\n>> Connection rejected; the client's version does not match the local version." << endl;
					cout << "->\tYour version: " << versionNum.str() << " - Remote version: " << remoteVersion.str() << endl;


					packet << (uint8)MSG_VERSION_MISMATCH << versionNum.major << versionNum.minor;
					Globals::Networking->sendSafe(packet);
					packet.clear();

					return ErrorCode::VersionMismatch;
				}

				packet << (uint8)MSG_VERSION_OK << versionNum.major << versionNum.minor;

				if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Status::Done)
				{
					cout << ">> An error occurred while confirming the connection with the client." << endl;
					return ErrorCode::ClientTimeout;
				}

				system("cls");
				cout << ">> Connected!" << endl;
			}
#pragma endregion
		}
		else
		{
#pragma region Client
			if (!Globals::Networking->isBound())
				cout << "<< Connecting to server at " << Address.ip << " on port " << Address.port << "..." << endl;

			if ((status = Globals::Networking->Connect(Address, false)) != sf::Socket::Done)
			{
				if (status == sf::Socket::Error)
				{
					cout << "<< A connection error has occurred." << endl;
					return errorCode = ErrorCode::ClientTimeout;
				}
				else if (status == sf::Socket::NotReady)
				{
					return ErrorCode::NotReady;
				}
			}


			if (!connected && CheckConnectOK())
			{
				packet << (uint8)MSG_VERSION_CHECK << versionNum.major << versionNum.minor;
				uint8 id;

				if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Done)
				{
					cout << "<< An error occurred while sending the version number!" << endl;
					return ErrorCode::NotReady;
				}

				packet.clear();

				if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
				{
					cout << ">> An error occurred while receiving version confirmation message." << endl;
					return ErrorCode::NotReady;
				}

				packet >> id;

				switch (id)
				{
				default:
					cout << ">> Received malformed packet from server!" << endl;
					break;

				case MSG_VERSION_MISMATCH:
					packet >> remoteVersion.major >> remoteVersion.minor;
					cout << "\n>> Connection rejected; the server's version does not match the local version." << endl;
					cout << "->\tYour version: " << versionNum.str() << " - Remote version: " << remoteVersion.str() << endl;
					return ErrorCode::VersionMismatch;
					break;

				case MSG_VERSION_OK:
					system("cls");
					cout << "<< Connected!" << endl;
					break;
				}
			}
#pragma endregion
		}

		PlayMusic("btl_sel.adx");
		PlayJingle("chao_k_net_fine.adx");

		connected = true;
		ApplySettings(true);

		return ErrorCode::None;
	}
	else
	{
		setMusic = false;
	}

	return ErrorCode::NotReady;
}


void Program::ApplySettings(const bool apply)
{
	if (apply)
		cout << "<> Applying code changes..." << endl;
	else
		cout << "<> Reverting code changes..." << endl;

	if (clientSettings.noSpecials)
		MemManage::nop2PSpecials(apply);
	if (clientSettings.isLocal)
		MemManage::swapInput(apply);
	if (clientSettings.KeepWindowActive)
		MemManage::keepActive(apply);

	if (!isServer)
	{
		MemManage::swapSpawn(apply);
		MemManage::swapCharsel(apply);
	}
	else
	{
		MemManage::swapSpawn(!apply);
		MemManage::swapCharsel(!apply);
	}
}

void Program::Disconnect(bool received, ErrorCode code)
{
	setMusic = false;

	cout << "<> Disconnecting..." << endl;
	Globals::Networking->Disconnect();

	ApplySettings(false);

	if (received)
		errorCode = code;

	PlayJingle("chao_k_net_fault.adx");
}