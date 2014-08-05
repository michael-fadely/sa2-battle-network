#include <iostream>
#include <cstdlib>		// for: srand and rand

#include "Common.h"
#include "Application.h"
#include "MemoryHandler.h"
#include "Networking.h"

#include "ReliablePacketHandler.h"
#include "ReliableID.h"

#include "PacketHandler.h"

using namespace std;
using namespace chrono;

PacketHandler::PacketHandler(Application::Program* program, uint timeout)
{
	cout << "Initializing packet handler..." << endl;

	// Store a pointer to the program, just in case
	Program = program;

	// Steal its socket and address struct
	Socket = program->Socket;
	Address = &program->Address;

	kaTimeout = timeout;

	// and now initialize the abstract memory
	AbstractMemory = new MemoryHandler(this, Program->isServer);

	// followed by simple variable initialization
	IncID = 0;
	LastID = 0;
	sentKeepalive = 0;
}

PacketHandler::~PacketHandler()
{
	//cout << "Killing packet handler..." << endl;

	Program = nullptr;
	Socket = nullptr;
	Address = nullptr;

	delete AbstractMemory;
}

const int PacketHandler::SendMsg(const bool isReliable)
{
	if (isReliable)
	{
		//reliableTimer = millisecs();
		netQueue.checkTimer = millisecs();
		netQueue.add(Socket);
	}

	return Socket->sendMsg(QSocket::StringToIntIP(Address->address), Address->port);
}

const uint PacketHandler::WriteReliable(const bool isRandom)
{
	uint id = 0;

	Socket->writeByte(RELIABLE_SEND);

	if (isRandom)
	{
		srand(millisecs());
		id = (uint)((uint)(rand()) % (uint)(pow(2, 10)));
	}
	else
	{
		if (IncID == UINT_MAX)
			IncID = 0;

		IncID++;
		id = IncID;
	}

	Socket->writeInt(id);
	return id;
}

const uint PacketHandler::Send()
{
	uint elapsed = millisecs();

	// Check for unreceived Reliable Messages
	netQueue.resend(this);

	AbstractMemory->GetFrame();

	// Check for input changes first and foremost
	// as quickly as possible. The function itself has
	// internal checks for menu, game state, etc.
	AbstractMemory->SendInput(Socket, sendTimer);


	//if (Duration(sendTimer) >= Program->UpdateInterval)
	if (!AbstractMemory->CheckFrame())
	{
		// Refresh system variables before continuing
		// otherwise this is all useless =P
		//AbstractMemory->Read();

		AbstractMemory->SendSystem(Socket);
		AbstractMemory->SendPlayer(Socket);
		AbstractMemory->SendMenu(Socket);
	}

	AbstractMemory->SetFrame();

	return Duration(elapsed);
}

const bool PacketHandler::ReliableHandler()
{
	uchar firstByte = (uchar)Socket->readByte();
	uint reliableID = 0;

	switch (firstByte)
	{
	default:
		return false;

	case RELIABLE_SEND: // Reliable message from the client which requires arrival confirmation
		reliableID = (uint)Socket->readInt();

		Socket->writeByte(RELIABLE_RECV);
		Socket->writeInt(reliableID);
		SendMsg();

		if (ridList.checkID(reliableID))
		{
			cout << "<> Relaible ID " << reliableID << " already received; not writing to memory." << endl;
			return true;
		}
		else
		{
			LastID = reliableID;
			ridList.addID(reliableID);
			return false;
		}

	case RELIABLE_RECV: // Arrival confirmation from the client that it has received the message
		reliableID = (uint)Socket->readInt();
		if (!netQueue.isEmpty())
			netQueue.del(reliableID);

		return false;
	}
}

const uint PacketHandler::Receive()
{
	AbstractMemory->GetFrame();

	uint elapsed = millisecs();
	ridList.update(5000);

	Socket->readAvail();
	if (Socket->msgAvail())
	{
		AbstractMemory->PreReceive();

		if (ReliableHandler())
			return Duration(elapsed);

		uchar msgNum = (uchar)Socket->readByte();
		uchar* msgTypes = new uchar[msgNum];

		for (uchar i = 0; i < msgNum; i++)
			msgTypes[i] = (uchar)Socket->readByte();

		for (uchar i = 0; i < msgNum; i++)
		{
			switch (msgTypes[i])
			{
			case MSG_KEEPALIVE:
				//cout << ">> Received keepalive!" << endl;
				recvKeepalive = millisecs();
				continue;

			case MSG_DISCONNECT:
				Program->Disconnect(true);
				i = msgNum;
				continue;

			case MSG_SCREWYOU:
			{
				system("color c");
				cout << ">>\a RECEIVED SCREWYOU MESSAGE!" << endl;

				uint msgLength = Socket->inMsgLength();
				char* buffer = new char[msgLength + sizeof(char)];
				buffer[msgLength] = '\0';

				memcpy(buffer, Socket->inMsgBuffer(), msgLength);

				cout << "->\t";
				for (uint b = 0; b < msgLength; b++)
					cout << " 0x" << hex << (ushort)buffer[b];
				cout << dec << endl;

				delete[] buffer;

				system("color 7");
				continue;
			}
			}

			CheckKeepalive();

			AbstractMemory->ReceiveSystem(Socket, msgTypes[i]);
			AbstractMemory->ReceiveInput(Socket, msgTypes[i]);
			AbstractMemory->ReceivePlayer(Socket, msgTypes[i]);
			AbstractMemory->ReceiveMenu(Socket, msgTypes[i]);
		}

		delete[] msgTypes;
		recvTimer = millisecs();

		return Duration(elapsed);
	}

	else if (!AbstractMemory->CheckFrame())
	{
		//cout << "\t\tNo Message Receive Duration: " << Duration(recvTimer) << endl;
		AbstractMemory->PostReceive();

		recvTimer = millisecs();
	}

	CheckKeepalive();
	return Duration(elapsed);
}

void PacketHandler::CheckKeepalive()
{
	if (Duration(kaTimer) >= 500)
	{
		//cout << "<>\t\tDuration since last keepalive: " << Duration(recvKeepalive) << endl;
		if (Duration(recvKeepalive) >= kaTimeout)
			Program->Disconnect(true, Application::ExitCode::ClientTimeout);

		kaTimer = millisecs();
	}
}

void PacketHandler::setStartTime(const uint time)
{
	recvKeepalive = time;
	kaTimer = time;
	AbstractMemory->setStartTime(time);
}