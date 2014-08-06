#pragma once

/*
	To-do:
	* Implement real exceptions
*/

#include <SFML\Network.hpp>
#include <mutex>
#include "PacketExtensions.h"

class PacketHandler
{
public:
	// Methods
	// De/Contstructor
	PacketHandler();
	~PacketHandler();

	// Listens for incoming connections and accepts them
	const sf::Socket::Status Listen(unsigned short port = 27015);
	// Connects to the address ip on the port port
	const sf::Socket::Status Connect(sf::IpAddress ip, unsigned short port);
	// Disconnects all sockets
	// Returns NotReady if none are connected
	const sf::Socket::Status Disconnect();

	// Returns the connected state of the packet handler.
	const bool isConnected() { return connected; }
	// Returns the host state of the packet handler.
	// Returns true if Listen() was called, and false otherwise
	// or after Connect() has been called.
	const bool isServer() { return host; }
	// Returns the time the last successful connection was established.
	// Returns 0 if none have been established yet.
	const unsigned int ConnectStartTime() { return start_time; }

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

	//inline void setSentKeepalive() { sentKeepalive = millisecs(); }
	//inline const unsigned int getSentKeepalive() { return sentKeepalive; }
	//void setStartTime(const unsigned int time);

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
	bool connected;
	bool host;
	unsigned int start_time;
	sf::TcpListener listener;
	RemoteAddress Address;


	//MemoryHandler* AbstractMemory;

	// Timers
	//unsigned int sendTimer;
	//unsigned int recvTimer;

	// Time the last keepalive message was received.
	//unsigned int recvKeepalive;
	// The time the last keepalive message was sent.
	//unsigned int sentKeepalive;
	// Time since the last keepalive check
	//unsigned int kaTimer;
	// The timeout for the keepalive system.
	//unsigned int kaTimeout;

	// Methods

	// Initializes the sockets and such.
	// Would be used for constructor overloads... if there WERE any!
	void Initialize();

	// Checks the packet for reliable flag(s) and responds
	// or returns true if the ID has already been received.
	const bool ReliableHandler();
	// Checks if the connection has timed out.
	void CheckKeepalive();
};