#pragma once

/*
	TODO:
	* Better exceptions / Exception handler
	* Multi-connection mode
	* Single-protocol modes (TCP/UDP)
*/

#include <SFML/Network.hpp>
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

	// Socket lock.
	// Used to enable use of mod loader hooks without
	// worrying about threads.
	std::mutex lockSafe, lockFast;
	// TCP socket.
	// Used to initiate connections and send order/context-sensitive data.
	sf::TcpSocket socketSafe;
	// UDP socket.
	// Used to quickly send frequently changing data.
	sf::UdpSocket socketFast;

	//
	//	Methods	
	//

	// Listens for incoming connections and accepts them.
	// This should be used by the server, not the client.
	// Blocks the calling thread if the parameter block is true.
	sf::Socket::Status Listen(const unsigned short port = 27015, const bool block = true);
	// Connects to the address and port in address.
	// This should be used by the client, not the server.
	// Blocks the calling thread if the parameter block is true.
	sf::Socket::Status Connect(RemoteAddress address, const bool block = true);
	// Connects to the address ip on the port port.
	// This should be used by the client, not the server.
	// Blocks the calling thread if the parameter block is true.
	sf::Socket::Status Connect(sf::IpAddress ip, const unsigned short port, const bool block = true);
	// Disconnects all sockets
	// Returns NotReady if none are connected.
	// If the parameter received is false (default), it sends a disconnect message,
	// otherwise it does not, and simply closes all sockets.
	sf::Socket::Status Disconnect(const bool received = false);

	// Returns the state of bound ports.
	bool isBound() { return bound; }
	// Returns the connection state of the packet handler.
	bool isConnected() { return connected; }
	// Returns the host state of the packet handler.
	// Returns true if Listen() was called, and false otherwise
	// or after Connect() has been called.
	bool isServer() { return host; }
	// Returns the time in milliseconds that the last successful connection was established.
	// Returns 0 if none have been established yet.
	unsigned int ConnectStartTime() { return start_time; }

	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate send function (safe/fast).
	sf::Socket::Status Send(PacketEx& packet);
	// Automatically pull information from PacketEx (isSafe)
	// and use the appropriate receive function (safe/fast).
	sf::Socket::Status Receive(PacketEx& packet, const bool block = false);

	// Send packet via TCP (safe)
	sf::Socket::Status sendSafe(sf::Packet& packet);
	// Receive packet via TCP (safe)
	// Blocks thread if parameter block is true
	sf::Socket::Status recvSafe(sf::Packet& packet, const bool block = false);
	// Send packet via UDP (fast)
	sf::Socket::Status sendFast(sf::Packet& packet);
	// Receive packet via UDP (fast)
	// Blocks thread if parameter block is true
	sf::Socket::Status recvFast(sf::Packet& packet, const bool block = false);

protected:
	//
	//	Members
	//

	// Value is true if ports have already been bound.
	bool bound;

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
	// Binds the UDP socket to the specified port.
	sf::Socket::Status Bind(const unsigned short port, const bool isServer);
	// Sets the time in milliseconds that the last successful connection was established.
	void SetConnectTime();
};