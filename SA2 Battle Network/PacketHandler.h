#pragma once

// TODO: Single-protocol modes (TCP/UDP)

#include <SFML/Network.hpp>
#include <deque>
#include "typedefs.h"
#include "PacketEx.h"

class PacketHandler
{
public:
	typedef int8 Node;
	PacketHandler();
	~PacketHandler();

	struct RemoteAddress
	{
		sf::IpAddress ip;
		ushort port;
	};

	struct Connection
	{
		Node node;
		sf::TcpSocket* tcpSocket;
		RemoteAddress udpAddress;
	};

	sf::Socket::Status Listen(ushort port, Node& node, bool block = true);
	sf::Socket::Status Connect(RemoteAddress remoteAddress, bool block = true);
	sf::Socket::Status Connect(sf::IpAddress ip, ushort port, bool block = true);
	void Disconnect();
	bool isBound() const { return bound; }
	bool isConnected() const { return connections.size() > 0; }
	auto ConnectionCount() const { return connections.size(); }
	bool isServer() const { return host; }
	ushort GetLocalPort() const { return udpSocket.getLocalPort(); }	
	auto Connections() const { return connections; }
	void SetRemotePort(Node node, ushort port);

	sf::Socket::Status Send(PacketEx& packet, Node node = -1, Node node_exclude = -1);
	sf::Socket::Status SendTCP(sf::Packet& packet, Node node = -1, Node node_exclude = -1);
	sf::Socket::Status SendUDP(sf::Packet& packet, Node node = -1, Node node_exclude = -1);
	sf::Socket::Status ReceiveTCP(sf::Packet& packet, Connection& connection, bool block = false) const;
	sf::Socket::Status ReceiveUDP(sf::Packet& packet, Node& node, RemoteAddress& remoteAddress, bool block = false);
	bool isConnectedAddress(RemoteAddress& remoteAddress) const;

protected:
	bool bound;
	bool host;
	sf::TcpListener listener;
	std::deque<Connection> connections;
	sf::UdpSocket udpSocket;
	Connection what;

	sf::Socket::Status bindPort(ushort port, bool isServer);
	Connection getConnection(Node node);
	void initCommunication();
	Node addConnection();
};
