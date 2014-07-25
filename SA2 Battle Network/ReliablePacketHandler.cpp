#include <iostream>

#include "Common.h"
#include "QuickSock.h"
#include "PacketHandler.h"

#include "ReliablePacketHandler.h"

using namespace std;

// ------------
// Packet class
// ------------

reliablePacket::reliablePacket(QSocket* Socket, const uint lifespan = 10000)
{
	uid = 0;
	this->lifespan = 0;
	timeAdded = 0;
	msgLength = Socket->outMsgLength();

	// Place the message in a buffer:
	message = new unsigned char[msgLength+sizeof(unsigned char)];
	message[Socket->outMsgSize()] = '\0';
	memcpy(message, Socket->outMsgBuffer(), msgLength);

	// and extract the UID
	memcpy(&uid, message+1, sizeof(uid));
	uid = ntohl(uid);

	// ... and of course, the lifespan of the message =P
	timeAdded = millisecs();
	this->lifespan = lifespan;
}

reliablePacket::~reliablePacket()
{
	if (message != nullptr)
	{
		delete[] this->message;
		this->message = nullptr;
	}
}

// ------------
// Queue class
// ------------

const bool reliableQueue::add(QSocket* Socket)
{
	if (Socket == nullptr) return false;

	reliablePacket* input = new reliablePacket(Socket);
	
	// Just to make it clear, we use the class name
	// as not to confuse it with std::find
	if (!reliableQueue::find(input))
	{
		deque.push_back(input);

		return true;
	}
	else
	{
		cout << "[reliableQueue::add] A message with the UID" << input->uid << "already exists in the deque; ignoring." << endl;

		// Delete the unused packet object.
		delete input;
	}
		
	return false;
}

const bool reliableQueue::del(const uint uid)
{
	// Local variable(s):
	bool response(false);
	//cout << "[reliableQueue::del] Deleting..." << endl;

	for (auto i = deque.begin(); i != deque.end();)
	{
		// If the current packet has the same UID as the one specified:
		if ((*i)->uid == uid)
		{
			// Delete the packet in question.
			delete *i;

			// Set the deletion response to true.
			response = true;

			// Move forward.
			i = deque.erase(i);
		}
		else
			++i;
	}

	if (!response)
		cout << "[reliableQueue::del] The UID supplied (" << uid << ") does not exist in the deque; ignoring." << endl;

	return response;
}

const bool reliableQueue::isDead()
{
	if (!isEmpty() && Duration(deque.front()->timeAdded) >= deque.front()->lifespan)
	{
		cout << "[reliableQueue::isDead] Packet exceeded lifespan (" << deque.front()->lifespan << "ms)." << endl;
		del(deque.front()->uid);

		return true;
	}
	else
		return false;
}

void reliableQueue::writeTopMsg(QSocket* Socket)
{
	Socket->writeBytes((const char*)deque.front()->message, deque.front()->msgLength);
	return;
}

void reliableQueue::resend(PacketHandler* networking)
{
	if (Duration(checkTimer) >= 250)
	{
		checkTimer = millisecs();
		if (!isEmpty() && !isDead())
		{
			cout << "[reliableQueue::resend()] Resending reliable message..." << endl;
			writeTopMsg(networking->Socket);
			networking->SendMsg();
			checkTimer = millisecs();
			return;
		}
		else
			return;
	}
	else
		return;
}