#include "stdafx.h"

#include <algorithm>
#include <Winsock2.h>
#include "PacketHandler.h"

using namespace std;

// TODO: Handle disconnects
// TODO: Handle Connections that have invalid nodes on send

PacketHandler::PacketHandler() : bound(false), host(false), what({})
{
	listener.setBlocking(false);
	udpSocket.setBlocking(false);
}
PacketHandler::~PacketHandler()
{
	Disconnect();
}

sf::Socket::Status PacketHandler::Listen(ushort port, Node& node, bool block)
{
	sf::Socket::Status result;

	if (!bound)
	{
		// First and foremost, we should bind the UDP socket.
		bindPort(port, true);

		// If blocking is enabled, listen until a connection is established.
		do
		{
			result = listener.listen(port);
		} while (block && result == sf::Socket::Status::NotReady);

		// If there was an error, throw an exception.
		// If the result is otherwise non-critical and blocking is disabled, return its result.
		if (result == sf::Socket::Status::Error)
			throw WSAGetLastError();
		if (!block && result != sf::Socket::Status::Done)
			return result;

		bound = true;
	}

	initCommunication();

	// Now attempt to accept the connection.
	do
	{
		result = listener.accept(*what.tcpSocket);
	} while (block && result == sf::Socket::Status::NotReady);

	// Once again, throw an exception if necessary,
	// otherwise simply return the result.
	if (result == sf::Socket::Status::Error)
		throw WSAGetLastError();
	if (result != sf::Socket::Status::Done)
		return result;

	// Set host state to true to ensure everything knows what to do.
	host = true;

	// Pull the remote address out of the socket
	// and store it for later use with UDP.
	what.udpAddress.ip = what.tcpSocket->getRemoteAddress();
	node = addConnection();

	return result;
}
sf::Socket::Status PacketHandler::Connect(sf::IpAddress ip, ushort port, bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (!isConnected())
	{
		// First, let's bind the UDP port.
		// We're going to need its local port to send to the server.
		if (!bound)
		{
			bindPort(port, false);
			bound = true;
		}

		what.udpAddress.ip = ip;
		what.udpAddress.port = port;

		initCommunication();

		// Now we attempt to connect to ip and port
		// If blocking is enabled, we wait until something happens.
		do
		{
			result = what.tcpSocket->connect(ip, port);
		} while (block && result == sf::Socket::Status::NotReady);

		// If there was an error, throw an exception.
		// If the result is otherwise non-critical and blocking is disabled, return its result.
		if (result == sf::Socket::Status::Error)
			throw WSAGetLastError();
		if (!block && result != sf::Socket::Status::Done)
			return result;

		// Even though host defaults to false, ensure that it is in fact false.
		host = false;

		addConnection();
	}

	return result;
}
void PacketHandler::Disconnect()
{
	for (auto& i : connections)
	{
		i.tcpSocket->disconnect();
		delete i.tcpSocket;
	}

	connections.clear();
	udpSocket.unbind();
	listener.close();

	bound = false;
}

void PacketHandler::Disconnect(Node node)
{
	if (connections.size() == 1)
		Disconnect();

	auto c = find_if(connections.begin(), connections.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (c == connections.end())
		return;

	c->tcpSocket->disconnect();
	connections.erase(c);
}

sf::Socket::Status PacketHandler::bindPort(ushort port, bool isServer)
{
	sf::Socket::Status result;

	if ((result = udpSocket.bind((isServer) ? port : sf::Socket::AnyPort)) != sf::Socket::Status::Done)
	{
		if (result == sf::Socket::Status::Error)
			throw WSAGetLastError();
	}

	return result;
}

PacketHandler::Connection PacketHandler::getConnection(Node node)
{
	auto c = find_if(connections.begin(), connections.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (c == connections.end())
		return {};

	return *c;
}

void PacketHandler::initCommunication()
{
	if (what.tcpSocket != nullptr)
		return;

	auto socket = new sf::TcpSocket();
	what.tcpSocket = socket;
	socket->setBlocking(false);
}

PacketHandler::Node PacketHandler::addConnection()
{
	auto node = host ? (Node)connections.size() + 1 : 0;
	what.node = node;
	connections.push_back(what);
	what = {};
	return node;
}

sf::Socket::Status PacketHandler::Connect(RemoteAddress remoteAddress, bool block)
{
	return Connect(remoteAddress.ip, remoteAddress.port, block);
}

void PacketHandler::SetRemotePort(Node node, ushort port)
{
	auto connection = find_if(connections.begin(), connections.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (connection == connections.end())
		throw exception("No connections exist with the specified node.");

	connection->udpAddress.port = port;
}

sf::Socket::Status PacketHandler::Send(PacketEx& packet, Node node, Node node_exclude)
{
	if (!packet.isEmpty())
		return packet.Protocol == nethax::Protocol::TCP ? SendTCP(packet, node, node_exclude) : SendUDP(packet, node, node_exclude);

	return sf::Socket::Status::NotReady;
}
sf::Socket::Status PacketHandler::SendTCP(sf::Packet& packet, Node node, Node node_exclude)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (node < 0)
	{
		for (auto& i : connections)
		{
			if (i.node == node_exclude)
				continue;

			result = i.tcpSocket->send(packet);

			if (result != sf::Socket::Status::Done)
				throw exception("Data send failure.");
		}
	}
	else
	{
		auto connection = getConnection(node);
		if (connection.tcpSocket == nullptr)
			throw exception("No connections exist with the specified node.");

		result = connection.tcpSocket->send(packet);
		if (result != sf::Socket::Status::Done)
			throw exception("Data send failure.");
	}

	return result;
}
sf::Socket::Status PacketHandler::SendUDP(sf::Packet& packet, Node node, Node node_exclude)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (node < 0)
	{
		for (auto& i : connections)
		{
			if (i.node == node_exclude)
				continue;

			result = udpSocket.send(packet, i.udpAddress.ip, i.udpAddress.port);

			if (result != sf::Socket::Status::Done)
				throw exception("Data send failure.");
		}
	}
	else
	{
		auto connection = getConnection(node);
		if (connection.tcpSocket == nullptr)
			throw exception("No connections exist with the specified node.");

		result = udpSocket.send(packet, connection.udpAddress.ip, connection.udpAddress.port);
		if (result != sf::Socket::Status::Done)
			throw exception("Data send failure.");
	}

	return result;

}

sf::Socket::Status PacketHandler::ReceiveTCP(sf::Packet& packet, Connection& connection, bool block) const
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (isConnected())
	{
		do
		{
			result = connection.tcpSocket->receive(packet);
		} while (block && result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw WSAGetLastError();
	}

	return result;
}
sf::Socket::Status PacketHandler::ReceiveUDP(sf::Packet& packet, Node& node, RemoteAddress& remoteAddress, bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (isConnected())
	{
		do
		{
			result = udpSocket.receive(packet, remoteAddress.ip, remoteAddress.port);
		} while (block && result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw WSAGetLastError();

		auto connection = std::find_if(connections.begin(), connections.end(), [remoteAddress](auto c)
		{
			return c.udpAddress.port == remoteAddress.port && c.udpAddress.ip == remoteAddress.ip;
		});

		if (connection == connections.end())
		{
			node = BroadcastNode;
		}
		else
		{
			node = connection->node;
		}
	}

	return result;
}

bool PacketHandler::isConnectedAddress(RemoteAddress& remoteAddress) const
{
	return std::any_of(connections.begin(), connections.end(), [remoteAddress](auto c)
	{
		return c.udpAddress.port == remoteAddress.port && c.udpAddress.ip == remoteAddress.ip;
	});
}
