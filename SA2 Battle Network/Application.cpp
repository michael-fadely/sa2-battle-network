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
	AbstractMemory = new MemoryHandler();
}

Program::~Program()
{
	delete AbstractMemory;
}


ExitCode Program::Connect()
{
	// Used only for connection loops.
	bool connected = false;
	sf::Packet packet;
	sf::Socket::Status status = sf::Socket::Status::Error;


	if (AbstractMemory->GetCurrentMenu() >= Menu::BATTLE)
	{
		if (!setMusic)
		{
			ChangeMusic("chao_k_net_connect.adx");
			setMusic = true;
		}

		if (isServer)
		{
#pragma region Server
			if (!Globals::Networking->isBound())
				cout << "\aHosting server on port " << Address.port << "..." << endl;

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

			while (!connected && AbstractMemory->GetCurrentMenu() >= Menu::BATTLE)
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
				cout << "\a>> Connected!" << endl;

				connected = true;
				return ExitCode::None;
			}
#pragma endregion
		}
		else
		{
#pragma region Client
			if (!Globals::Networking->isBound())
				cout << "\a\a<< Connecting to server at " << Address.ip << " on port " << Address.port << "..." << endl;

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


			while (!connected && AbstractMemory->GetCurrentMenu() >= Menu::BATTLE)
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
					cout << "\a<< Connected!" << endl;
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


void Program::ApplySettings()
{
	MemManage::nopP2Input(true);

	if (clientSettings.noSpecials)
		MemManage::nop2PSpecials();
	if (clientSettings.isLocal)
		MemManage::swapInput(true);
	if (clientSettings.KeepWindowActive)
		MemManage::keepActive();

	if (!isServer)
	{
		MemManage::swapSpawn(true);
		MemManage::swapCharsel(true);
	}
	else
	{
		MemManage::swapSpawn(false);
		MemManage::swapCharsel(false);
	}
}

const ExitCode Program::RunLoop()
{
	AbstractMemory->Initialize();
	if (Globals::Networking->isConnected())
	{
		PlayMusic("btl_sel.adx");
		PlayJingle(0, "chao_k_net_fine.adx");

		exitCode = ExitCode::None;

		//uint sendElapsed, recvElapsed;
		//uint titleTimer = millisecs();
		//stringstream title;

		//uint frame, framecount;

		while (Globals::Networking->isConnected())
		{
			AbstractMemory->RecvLoop();
			AbstractMemory->SendLoop();

			// Check to see if we should disconnect
			if (!(AbstractMemory->GetCurrentMenu() >= Menu::BATTLE))
				break;

			AbstractMemory->SetFrame();

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
	if (received)
	{
		cout << "<> Reverting swaps..." << endl;

		MemManage::nopP2Input(false);

		if (clientSettings.noSpecials)
			MemManage::nop2PSpecials();
		if (clientSettings.isLocal)
			MemManage::swapInput(false);
		if (clientSettings.KeepWindowActive)
			MemManage::keepActive();

		if (!isServer)
		{
			MemManage::swapSpawn(false);
			MemManage::swapCharsel(false);
		}
		else
		{
			MemManage::swapSpawn(true);
			MemManage::swapCharsel(true);
		}

		exitCode = code;
	}

	MemManage::waitFrame();
	PlayJingle(0, "chao_k_net_fault.adx");
}