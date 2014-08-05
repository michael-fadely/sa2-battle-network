#pragma once

/*
//	"External" and "Internal refers to whether or not
//	the member (pointer or not) was created from within
//	this object or if a pointer was simply passed into it.
*/
#include <SFML\Network.hpp>
#include <mutex>
#include "PacketExtensions.h"

namespace Application { class Program; }
class MemoryHandler;

class PacketHandler
{
public:
	// Methods
	// De/Contstructor
	PacketHandler();
	~PacketHandler();

	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate send function.
	const sf::Socket::Status Send(PacketEx& packet);
	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate receive function.
	const sf::Socket::Status Receive(PacketEx& packet);

	const sf::Socket::Status sendSafe(sf::Packet& packet);
	const sf::Socket::Status recvSafe(sf::Packet& packet, const bool block = false);
	const sf::Socket::Status sendFast(sf::Packet& packet);
	const sf::Socket::Status recvFast(sf::Packet& packet, const bool block = false);

	const sf::Socket::Status Connect();

	const bool isConnected() { return connected; }

	//inline void setSentKeepalive() { sentKeepalive = millisecs(); }
	//inline const unsigned int getSentKeepalive() { return sentKeepalive; }

	void setStartTime(const unsigned int time);

	std::mutex safeLock, fastLock;
	sf::TcpSocket safeSocket;
	sf::UdpSocket fastSocket;

	struct RemoteAddress
	{
		sf::IpAddress ip;
		unsigned short port;
	};

protected:
	// Members
	sf::TcpListener listener;
	RemoteAddress Address;

	bool connected;

	//MemoryHandler* AbstractMemory;

	// Timers
	unsigned int sendTimer;
	unsigned int recvTimer;

	// Time the last keepalive message was received.
	//unsigned int recvKeepalive;
	// The time the last keepalive message was sent.
	//unsigned int sentKeepalive;
	// Time since the last keepalive check
	//unsigned int kaTimer;
	// The timeout for the keepalive system.
	//unsigned int kaTimeout;

	// Methods
	void Initialize();

	// Checks the packet for reliable flag(s) and responds
	// or returns true if the ID has already been received.
	const bool ReliableHandler();
	// Checks if the connection has timed out.
	void CheckKeepalive();
};