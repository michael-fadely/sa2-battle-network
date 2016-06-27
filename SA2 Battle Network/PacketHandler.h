#pragma once

// TODO: Single-protocol modes (TCP/UDP)

#include <SFML/Network.hpp>
#include <deque>
#include "typedefs.h"
#include "PacketEx.h"

class PacketHandler
{
public:
	PacketHandler();
	~PacketHandler();

	struct RemoteAddress
	{
		sf::IpAddress ip;
		ushort port;
	};

	struct Connection
	{
		sf::Int8 node;
		sf::TcpSocket* tcpSocket;
		RemoteAddress udpAddress;
	};

	sf::Socket::Status Listen(ushort port, bool block = true);
	sf::Socket::Status Connect(RemoteAddress remoteAddress, bool block = true);
	sf::Socket::Status Connect(sf::IpAddress ip, ushort port, bool block = true);
	void Disconnect();
	bool isBound() const { return bound; }
	bool isConnected() const { return connections.size() > 0; }
	auto ConnectionCount() const { return connections.size(); }
	bool isServer() const { return host; }
	ushort GetLocalPort() const { return udpSocket.getLocalPort(); }	
	auto Connections() const { return connections; }
	void SetRemotePort(int8 node, ushort port);

	sf::Socket::Status Send(PacketEx& packet, int8 node = -1, int8 node_exclude = -1);
	sf::Socket::Status SendTCP(sf::Packet& packet, int8 node = -1, int8 node_exclude = -1);
	sf::Socket::Status SendUDP(sf::Packet& packet, int8 node = -1, int8 node_exclude = -1);
	sf::Socket::Status ReceiveTCP(sf::Packet& packet, Connection& connection, bool block = false) const;
	sf::Socket::Status ReceiveUDP(sf::Packet& packet, int8& node, RemoteAddress& remoteAddress, bool block = false);
	bool isConnectedAddress(RemoteAddress& remoteAddress) const;

protected:
	// Value is true if ports have already been bound.
	bool bound;
	// Value is true if Listen() was called, and false otherwise
	// or after Connect() has been called.
	bool host;
	// The listener used to establish a connection
	sf::TcpListener listener;

	Connection what;
	std::deque<Connection> connections;
	sf::UdpSocket udpSocket;

	// Binds the UDP socket to the specified port.
	sf::Socket::Status bindPort(ushort port, bool isServer);
	Connection getConnection(int8 node);
};
