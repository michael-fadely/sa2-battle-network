#include "stdafx.h"

#include <Winsock2.h>
#include "PacketHandler.h"

using namespace std;

PacketHandler::PacketHandler() : bound(false), connected(false), host(false), Address({})
{
	listener.setBlocking(false);
	socketSafe.setBlocking(false);
	socketFast.setBlocking(false);
}
PacketHandler::~PacketHandler()
{
	Disconnect();
}

sf::Socket::Status PacketHandler::Listen(const ushort port, const bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;
	int error = 0;

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
			throw error = WSAGetLastError();
		if (!block && result != sf::Socket::Status::Done)
			return result;

		bound = true;
	}

	// Now attempt to accept the connection.
	do
	{
		result = listener.accept(socketSafe);
	} while (block && result == sf::Socket::Status::NotReady);

	// Once again, throw an exception if necessary,
	// otherwise simply return the result.
	if (result == sf::Socket::Status::Error)
		throw error = WSAGetLastError();
	if (result != sf::Socket::Status::Done)
		return result;

	// Pull the remote address out of the socket
	// and store it for later use with UDP.
	Address.ip = socketSafe.getRemoteAddress();

	connected = true;	// Set connection state to true to allow use of communication functions
	host = true;		// Set host state to true to ensure everything knows what to do.

	return result;
}
sf::Socket::Status PacketHandler::Connect(sf::IpAddress ip, const ushort port, const bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (!connected)
	{
		int error = 0;
		// First, let's bind the UDP port.
		// We're going to need its local port to send to the server.
		if (!bound)
		{
			bindPort(port, false);
			bound = true;
		}

		// Assign the address you klutz! STOP FORGETTING!
		Address.ip = ip;
		Address.port = port;

		// Now we attempt to connect to ip and port
		// If blocking is enabled, we wait until something happens.
		do
		{
			result = socketSafe.connect(ip, port);
		} while (block && result == sf::Socket::Status::NotReady);

		// If there was an error, throw an exception.
		// If the result is otherwise non-critical and blocking is disabled, return its result.
		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
		if (!block && result != sf::Socket::Status::Done)
			return result;

		// Set connection state to true to allow use of communication functions.
		connected = true;
		// Even though host defaults to false, ensure that it is in fact false.
		host = false;
	}

	return result;
}
void PacketHandler::Disconnect()
{
	socketSafe.disconnect();
	socketFast.unbind();
	listener.close();

	bound = false;
	connected = false;
}
sf::Socket::Status PacketHandler::bindPort(const ushort port, const bool isServer)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if ((result = socketFast.bind((isServer) ? port : sf::Socket::AnyPort)) != sf::Socket::Status::Done)
	{
		int error = 0;
		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}

sf::Socket::Status PacketHandler::Connect(RemoteAddress remoteAddress, const bool block)
{
	return Connect(remoteAddress.ip, remoteAddress.port, block);
}

/// <summary>
/// Sets the remote port for the UDP socket.
/// </summary>
/// <param name="port">The port.</param>
void PacketHandler::SetRemotePort(ushort port)
{
	Address.port = port;
}

sf::Socket::Status PacketHandler::Send(PacketEx& packet)
{
	if (!packet.isEmpty())
		return packet.Protocol == nethax::Protocol::TCP ? SendSafe(packet) : SendFast(packet);

	return sf::Socket::Status::NotReady;
}
sf::Socket::Status PacketHandler::Receive(PacketEx& packet, RemoteAddress& remoteAddress, const bool block)
{
	return packet.Protocol == nethax::Protocol::TCP ? ReceiveSafe(packet, block) : ReceiveFast(packet, remoteAddress, block);
}

sf::Socket::Status PacketHandler::SendSafe(sf::Packet& packet)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (connected)
	{
		int error = 0;

		do
		{
			result = socketSafe.send(packet);
		} while (result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}
sf::Socket::Status PacketHandler::ReceiveSafe(sf::Packet& packet, const bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (connected)
	{
		int error = 0;

		do
		{
			result = socketSafe.receive(packet);
		} while (block && result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}
sf::Socket::Status PacketHandler::SendFast(sf::Packet& packet)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (connected)
	{
		int error = 0;

		do
		{
			result = socketFast.send(packet, Address.ip, Address.port);
		} while (result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}
sf::Socket::Status PacketHandler::ReceiveFast(sf::Packet& packet, RemoteAddress& remoteAddress, const bool block)
{
	sf::Socket::Status result = sf::Socket::Status::NotReady;

	if (connected)
	{
		int error = 0;

		do
		{
			result = socketFast.receive(packet, remoteAddress.ip, remoteAddress.port);
		} while (block && result == sf::Socket::Status::NotReady);

		if (result == sf::Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}

bool PacketHandler::isConnectedAddress(RemoteAddress& remoteAddress) const
{
	return remoteAddress.ip == Address.ip && remoteAddress.port == Address.port;
}
