#include <string>
#include <sstream>
#include <chrono>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"
#include "Networking.h"
#include "QuickSock.h"
#include "PacketHandler.h"
#include "ReliablePacketHandler.h"
#include "ReliableID.h"
#include "MemoryManagement.h"

#include "Application.h"

using namespace std;
using namespace chrono;

using namespace Application;

const std::string Version::str()
{
	stringstream out;
	out << (ushort)major << "." << (ushort)minor;
	return out.str();
}

Version Program::versionNum = { 2, 7 };
const string Program::version = "SA2:BN Version: " + Program::versionNum.str();

Program::Program(bool server, clientAddress& address, Settings& settings, uint timeout)
{
	isServer = server;
	ConnectionStart = 0;

	Address.address = address.address;
	Address.port = address.port;

	// Oh man. This is so terrible.
	memcpy(&this->settings, &settings, sizeof(Settings));

	QSocket::initSockets();
	Socket = new QSocket(1024);

	Networking = new PacketHandler(this, timeout);

	return;
}

Program::~Program()
{
	delete Socket;
	QSocket::deinitSockets();

	delete Networking;

	cout << "<> o/" << endl;

	return;
}


ExitCode Program::Connect()
{
	if (isServer)
	{
		cout << "Hosting server on port " << Address.port << "..." << endl;
		Socket->host(Address.port);
		cout << "Waiting for connections..." << endl;
	}
	else
	{
		cout << "Connecting to server at " << Address.address << " on port " << Address.port << "..." << endl;
		Socket->connect(Address.address.c_str(), Address.port);
	}

	isConnected = false;
	while (!isConnected)
	{
		Socket->readAvail();

		if (Socket->msgAvail())
		{
			if (Socket->readByte() == MSG_ESTABLISH)
			{
				SendInitMsg();

				remoteVersion.major = (uchar)Socket->readByte();
				remoteVersion.minor = (uchar)Socket->readByte();

				if (memcmp(&versionNum, &remoteVersion, sizeof(Version)) == 0)
				{
					ConnectionStart = millisecs();
					isConnected = true;

					if (isServer)
					{
						Address.address = QSocket::IntToStringIP(Socket->msgIP());
						Address.port = Socket->msgPort();
					}

					break;
				}
				else
				{
					if (isServer)
					{
						cout << "\n>> Connection rejected; the client's version does not match the local version." << endl;
						cout << "->\tYour version: " << versionNum.str() << " - Client version: " << remoteVersion.str() << endl;
					}
					else
						return exitCode = ExitCode::VersionMismatch;
				}
			}
		}
		else if (!isServer)
		{
			SendInitMsg();
		}
		SleepFor((milliseconds)250);
	}

	system("cls");

	if (isConnected)
	{
		if (isServer)
			cout << "Client connected from " << Address.address << " on port " << Address.port << "!" << endl;
		else
			cout << "Connected!" << endl;
	}

	ConnectionStart = millisecs();
	Networking->setStartTime(ConnectionStart);

	return ExitCode::None;
}


void Program::ApplySettings()
{
	MemManage::nopP2Input(true);

	if (settings.noSpecials)
		MemManage::nop2PSpecials();
	if (settings.isLocal)
		MemManage::swapInput(true);
	if (settings.KeepWindowActive)
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

/*
// Deprecated
bool Program::isProcessRunning()
{
DWORD exitcode = 0;
GetExitCodeProcess(sa2bn::Globals::ProcessID, &exitcode);

if (exitcode == STILL_ACTIVE)
return true;
else
{
CloseHandle(sa2bn::Globals::ProcessID);
exitCode = ExitCode::GameTerminated;
return false;
}
}
*/

ExitCode Program::RunLoop()
{
	if (isConnected)
	{
		exitCode = ExitCode::None;

		uint sendElapsed, recvElapsed;
		uint titleTimer = millisecs();
		stringstream title;

		uint frame, framecount;

		while (isConnected)
		{
			frame = MemManage::getFrameCount();

			// Receiver
			recvElapsed = Networking->Receive();

			// Sender
			sendElapsed = Networking->Send();

			if (Duration(titleTimer) >= 250)
			{
				framecount = MemManage::getElapsedFrames(frame);
				title.str("");
				title << version << " [RECV: " << recvElapsed << " | SEND: " << sendElapsed << " | FRAMES: " << framecount << "]";
				SetConsoleTitleA(title.str().c_str());
				titleTimer = millisecs();
			}

			//if (MemManage::getFrameCount() == frame)
			//	MemManage::waitFrame(1, frame);
			SleepFor((milliseconds)1);
		}

		return exitCode;
	}
	else
	{
		return exitCode;
	}
}

void Program::Disconnect(bool received, ExitCode code)
{
	cout << "<> Disconnecting..." << endl;
	if (received)
	{
		isConnected = false;
		cout << "<> Reverting swaps..." << endl;

		MemManage::nopP2Input(false);

		if (settings.noSpecials)
			MemManage::nop2PSpecials();
		if (settings.isLocal)
			MemManage::swapInput(false);
		if (settings.KeepWindowActive)
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
	else
	{
		bool Received = false;
		uint time = millisecs();
		uint id = 0;

		// Might as well
		Networking->netQueue.deque.clear();

		// Store the ID used and send off the reliable message
		id = Networking->WriteReliable(); Socket->writeByte(1);
		Socket->writeByte(MSG_DISCONNECT);

		Networking->SendMsg(true);

		while (!Received)
		{
			if (Duration(time) >= 10000)
				break;

			Socket->readAvail();
			if (Socket->msgAvail())
			{
				if (!Networking->ReliableHandler() && Networking->LastID == id)
				{
					Received = true;
					isConnected = false;
					break;
				}
			}

			Networking->netQueue.resend(Networking);
		}

		isConnected = false;
		return;
	}
}

bool Program::OnEnd()
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

inline void Program::SendInitMsg()
{
	Socket->writeByte(MSG_ESTABLISH);
	Socket->writeByte(versionNum.major); Socket->writeByte(versionNum.minor);
	Socket->sendMsg();

	return;
}