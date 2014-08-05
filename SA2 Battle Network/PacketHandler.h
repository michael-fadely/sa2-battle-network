#pragma once

/*
//	"External" and "Internal refers to whether or not
//	the member (pointer or not) was created from within
//	this object or if a pointer was simply passed into it.
*/
#include "Networking.h"
#include "ReliablePacketHandler.h"
#include "ReliableID.h"

namespace Application { class Program; }
class MemoryHandler;

class PacketHandler
{
public:
	friend class reliableQueue;
	friend class Application::Program;
	// Methods
	// De/Contstructor
	PacketHandler(Application::Program* program, unsigned int timeout);
	~PacketHandler();

	const unsigned int Send();
	const unsigned int Receive();

	const int SendMsg(const bool isReliable=false);
	const unsigned int WriteReliable(const bool isRandom=false);

	inline void setSentKeepalive() { sentKeepalive = millisecs(); }
	inline const unsigned int getSentKeepalive() { return sentKeepalive; }

	void setStartTime(const unsigned int time);

private:
	// Members
	// "External"
	Application::Program* Program;
	QSocket* Socket;
	clientAddress* Address;

	// Reliable Networking Handlers
	// "Internal"
	reliableQueue	netQueue;
	reliableID		ridList;

	MemoryHandler* AbstractMemory;

	// Timers
	unsigned int sendTimer;
	unsigned int recvTimer;

	// Time the last keepalive message was received.
	unsigned int recvKeepalive;
	// The time the last keepalive message was sent.
	unsigned int sentKeepalive;
	// Time since the last keepalive check
	unsigned int kaTimer;
	// The timeout for the keepalive system.
	unsigned int kaTimeout;

	// Reliable System IDs
	// This is used for the new Incremental Reliable ID system.
	// No more randomization! (although still supported (but should be rewritten))
	unsigned int IncID;

	// The last Reliable ID received.
	unsigned int LastID;

	// Methods

	// Checks the packet for reliable flag(s) and responds
	// or returns true if the ID has already been received.
	const bool ReliableHandler();
	// Checks if the connection has timed out.
	void CheckKeepalive();
};