#pragma once

/*
	To-do:
	* Implement real exceptions
	* Make listener non-blocking
*/

#include <SFML\Network.hpp>
#include <mutex>
#include "PacketExtensions.h"

class PacketHandler
{
public:
	PacketHandler();
	~PacketHandler();

	//
	//	Members
	//

	struct RemoteAddress
	{
		sf::IpAddress ip;
		unsigned short port;
	};

	std::mutex safeLock, fastLock;
	sf::TcpSocket safeSocket;
	sf::UdpSocket fastSocket;

	//
	//	Methods	
	//

	// Listens for incoming connections and accepts them
	const sf::Socket::Status Listen(const unsigned short port = 27015);
	// Connects to the address and port in address
	const sf::Socket::Status Connect(RemoteAddress address);
	// Connects to the address ip on the port port
	const sf::Socket::Status Connect(sf::IpAddress ip, const unsigned short port);
	// Disconnects all sockets
	// Returns NotReady if none are connected
	const sf::Socket::Status Disconnect();

	// Returns the connected state of the packet handler.
	const bool isConnected() { return connected; }
	// Returns the host state of the packet handler.
	// Returns true if Listen() was called, and false otherwise
	// or after Connect() has been called.
	const bool isServer() { return host; }
	// Returns the time in milliseconds that the last successful connection was established.
	// Returns 0 if none have been established yet.
	const unsigned int ConnectStartTime() { return start_time; }

	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate send function.
	const sf::Socket::Status Send(PacketEx& packet);
	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate receive function.
	const sf::Socket::Status Receive(PacketEx& packet, const bool block = false);

	// Send packet via TCP (safe)
	const sf::Socket::Status sendSafe(sf::Packet& packet);
	// Receive packet via TCP (safe)
	// Blocks thread if parameter block is true
	const sf::Socket::Status recvSafe(sf::Packet& packet, const bool block = false);
	// Send packet via UDP (fast)
	const sf::Socket::Status sendFast(sf::Packet& packet);
	// Receive packet via UDP (fast)
	// Blocks thread if parameter block is true
	const sf::Socket::Status recvFast(sf::Packet& packet, const bool block = false);


protected:
	//
	//	Members
	//

	// Value is true if a connection was successfully established.
	bool connected;
	// Value is true if Listen() was called, and false otherwise
	// or after Connect() has been called.
	bool host;
	// The time in milliseconds that the last successful connection was established.
	// Value is 0 if none have been established yet.
	unsigned int start_time;
	// The listener used to establish a connection
	sf::TcpListener listener;
	// The address/port to connect to,
	// or port to listen on.
	RemoteAddress Address;

	//
	//	Methods	
	//

	// Initializes the sockets and such.
	// Would be used for constructor overloads... if there WERE any!
	void Initialize();
	
	// Sets the time in milliseconds that the last successful connection was established.
	void SetConnectTime();
};