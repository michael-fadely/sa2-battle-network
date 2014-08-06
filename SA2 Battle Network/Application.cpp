#include <string>
#include <sstream>
#include <chrono>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <SFML\Network.hpp>

#include "Common.h"
#include "Networking.h"

#include "PacketHandler.h"
#include "MemoryManagement.h"
#include "MemoryHandler.h"

#include "Application.h"

using namespace std;
using namespace chrono;

using namespace Application;
using namespace sa2bn;

Version Program::versionNum = { 3, 0 };
const string Program::version = "SA2:BN Version: " + Program::versionNum.str();

const std::string Version::str()
{
	stringstream out;
	out << (ushort)major << "." << (ushort)minor;
	return out.str();
}


Program::Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address) :
exitCode(ExitCode::None),
clientSettings(settings),
remoteVersion({ 3, 0 }),
isServer(host),
Address(address)
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

	if (isServer)
	{
		cout << "\aHosting server on port " << Address.port << "..." << endl;

		if ((status = Globals::Networking->Listen(Address.port)) != sf::Socket::Done)
		{
			cout << "An error occurred while trying to listen for connections on port " << Address.port << endl;
			return exitCode = ExitCode::ClientTimeout;
		}

		while (!connected)
		{

			if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
			{
				cout << "An error occurred while waiting for version number." << endl;
				continue;
			}

			uchar id;
			packet >> id;
			if (id != MSG_VERSION_CHECK)
			{
				cout << "Received malformed packet from client!" << endl;
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
				cout << Globals::Networking->isConnected() << " An error occurred while confirming the connection with the client." << endl;
				continue;
			}

			system("cls");
			cout << "\aConnected!" << endl;

			connected = true;
			break;
		}
	}
	else
	{
		cout << "\a\aConnecting to server at " << Address.ip << " on port " << Address.port << "..." << endl;

		if ((status = Globals::Networking->Connect(Address)) != sf::Socket::Done)
		{
			cout << "A connection error has occurred." << endl;
			return exitCode = ExitCode::ClientTimeout;
		}


		while (!connected)
		{
			packet << (unsigned char)MSG_VERSION_CHECK << versionNum.major << versionNum.minor;
			uchar id;

			if ((status = Globals::Networking->sendSafe(packet)) != sf::Socket::Done)
			{
				cout << "An error occurred while sending the version number!" << endl;
				continue;
			}

			packet.clear();

			if ((status = Globals::Networking->recvSafe(packet, true)) != sf::Socket::Done)
			{
				cout << "An error occurred while receiving version confirmation message." << endl;
				continue;
			}

			packet >> id;

			switch (id)
			{
			default:
				cout << "Received malformed packet from server!" << endl;
				break;

			case MSG_VERSION_MISMATCH:
				packet >> remoteVersion.major >> remoteVersion.minor;
				cout << "\n>> Connection rejected; the client's version does not match the local version." << endl;
				cout << "->\tYour version: " << versionNum.str() << " - Remote version: " << remoteVersion.str() << endl;
				break;

			case MSG_VERSION_OK:
				system("cls");
				cout << "\aConnected!" << endl;
				connected = true;
				break;
			}
		}
	}

	return ExitCode::None;
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
	if (Globals::Networking->isConnected())
	{
		exitCode = ExitCode::None;

		//uint sendElapsed, recvElapsed;
		//uint titleTimer = millisecs();
		//stringstream title;

		//uint frame, framecount;

		while (Globals::Networking->isConnected())
		{
			AbstractMemory->RecvLoop();
			AbstractMemory->SendLoop();
			SleepFor((milliseconds)1);
		}

	}
	return exitCode;
}

void Program::Disconnect(bool received, ExitCode code)
{
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
}

const bool Program::OnEnd()
{
	stringstream winMessage, winTitle;
	winMessage << "";
	winTitle << "SA2:BN - ";

	switch (exitCode)
	{
	case ExitCode::ClientDisconnect:
		winTitle << "Connection terminated";
		winMessage << "The other player has exited the game.";
		break;

	case ExitCode::ClientTimeout:
		winTitle << "Connection timeout";
		winMessage << "The connection to the other player has timed out.";
		break;

	case ExitCode::GameTerminated:
		winTitle << "Game terminated";
		winMessage << "The game has been terminated (or it crashed).";
		break;

	case ExitCode::VersionMismatch:
		winTitle << "Connection rejected: Version mismatch";
		winMessage << "Connection rejected: the version number received from the client does not match the local version." << endl << endl;
		winMessage << "Your version: " << versionNum.str() << endl;
		winMessage << "Server version: " << remoteVersion.str();
		break;

	case ExitCode::None:
		winTitle << "CRAP";
		winMessage << "PLZ NO. Report to SF94/Morph on Sonic Retro.";
		break;
	}

	winMessage << "\n\nWould you like to restart SA2:BN with the same settings?"
		<< "\nChoosing \"No\" will exit the program.";

	int mbResult = MessageBoxA(NULL, winMessage.str().c_str(), winTitle.str().c_str(), MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);

	if (mbResult == IDNO)
		return false;

	return true;
}