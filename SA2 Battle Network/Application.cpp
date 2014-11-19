#include <iostream>
#include <string>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <SFML\Network.hpp>

#include "Common.h"
#include "Networking.h"

#include "PacketHandler.h"
#include "MemoryManagement.h"
#include "MemoryHandler.h"
#include "ModLoaderExtensions.h"
#include "CommonEnums.h"

#include "Application.h"

// TODO: Replace music filename strings with constants or read them from a file.

using namespace std;
using namespace chrono;

using namespace Application;
using namespace sa2bn;


void ChangeMusic(const char* song)
{
	StopMusic();
	PlayMusic(song);
	ResetMusic();
}

Version Program::versionNum = { 3, 0 };
const string Program::version = "SA2:BN Version: " + Program::versionNum.str();

const std::string Version::str()
{
	return to_string(major) + "." + to_string(minor);
}


Program::Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address) :
exitCode(ExitCode::None),
clientSettings(settings),
remoteVersion({ 3, 0 }),
isServer(host),
Address(address),
setMusic(false)
{
	memory = new MemoryHandler();
}

Program::~Program()
{
	delete memory;
}


ExitCode Program::Connect()
{


	if (memory->GetCurrentMenu() >= Menu::BATTLE)
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
				cout << "Hosting server on port " << Address.port << "..." << endl;

			if ((status = Globals::Networking->Listen(Address.port, false)) != sf::Socket::Done)
			{
				if (status == sf::Socket::Error)
				{
					cout << "An error occurred while trying to listen for connections on port " << Address.port << endl;
					return exitCode = ExitCode::ClientTimeout;
				}
				else if (status == sf::Socket::NotReady)
				{
					return ExitCode::NotReady;
				}
			}

			while (!connected && memory->GetCurrentMenu() >= Menu::BATTLE)
			{
				if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
				{
					cout << ">> An error occurred while waiting for version number." << endl;
					continue;
				}

				uchar id;
				packet >> id;
				if (id != MSG_VERSION_CHECK)
				{
					cout << ">> Received malformed packet from client!" << endl;
					continue;
				}

				packet >> remoteVersion.major >> remoteVersion.minor;
				packet.clear();

				if (memcmp(&versionNum, &remoteVersion, sizeof(Version)) != 0)
				{
					Globals::Networking->Disconnect();
					cout << "\n>> Connection rejected; the client's version does not match the local version." << endl;
					cout << "->\tYour version: " << versionNum.str() << " - Remote version: " << remoteVersion.str() << endl;


					packet << (uchar)MSG_VERSION_MISMATCH << versionNum.major << versionNum.minor;
					Globals::Networking->sendSafe(packet);
					packet.clear();

					continue;
				}

				packet << (uchar)MSG_VERSION_OK << versionNum.major << versionNum.minor;

				if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Status::Done)
				{
					cout << ">> An error occurred while confirming the connection with the client." << endl;
					continue;
				}

				system("cls");
				cout << ">> Connected!" << endl;

				connected = true;
				return ExitCode::None;
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
					return exitCode = ExitCode::ClientTimeout;
				}
				else if (status == sf::Socket::NotReady)
				{
					return ExitCode::NotReady;
				}
			}


			while (!connected && memory->GetCurrentMenu() >= Menu::BATTLE)
			{
				packet << (uchar)MSG_VERSION_CHECK << versionNum.major << versionNum.minor;
				uchar id;

				if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Done)
				{
					cout << "<< An error occurred while sending the version number!" << endl;
					continue;
				}

				packet.clear();

				if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
				{
					cout << ">> An error occurred while receiving version confirmation message." << endl;
					continue;
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
					break;

				case MSG_VERSION_OK:
					system("cls");
					cout << "<< Connected!" << endl;
					connected = true;
					return ExitCode::None;
				}
			}
#pragma endregion
		}
	}
	else
	{
		setMusic = false;
	}
	return ExitCode::NotReady;
}


void Program::ApplySettings(const bool apply)
{
	MemManage::nopP2Input(apply);

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

const ExitCode Program::RunLoop()
{
	memory->Initialize();
	if (Globals::Networking->isConnected())
	{
		ApplySettings(true);

		PlayMusic("btl_sel.adx");
		PlayJingle(0, "chao_k_net_fine.adx");

		exitCode = ExitCode::None;

		while (Globals::Networking->isConnected())
		{
			memory->RecvLoop();
			memory->SendLoop();

			// Check to see if we should disconnect
			if (!(memory->GetCurrentMenu() >= Menu::BATTLE))
				break;

			memory->SetFrame();

			// IN CASE OF SLOW, COMMENT FOR SPEED DEMON
			SleepFor((milliseconds)1);
		}
		Disconnect(false, ExitCode::NotReady);
	}
	return exitCode;
}

void Program::Disconnect(bool received, ExitCode code)
{
	setMusic = false;

	cout << "<> Disconnecting..." << endl;
	Globals::Networking->Disconnect();

	ApplySettings(false);

	if (received)
		exitCode = code;

	MemManage::waitFrame();
	PlayJingle(0, "chao_k_net_fault.adx");
}