#include <iostream>
#include <Winsock2.h>

#include "Common.h"		// millisecs(), LazyTypedefs
#include "Networking.h"
#include "PacketHandler.h"


using namespace std;
using namespace sf;

#pragma region PacketHandler

PacketHandler::PacketHandler() : start_time(0), host(false), connected(false), Address({})
{
	Initialize();
}
PacketHandler::~PacketHandler()
{
	Disconnect();
	safeSocket.disconnect();
}
void PacketHandler::Initialize()
{
	cout << "<> Initializing packet handler..." << endl;
	//recvTimer = sendTimer = 0;
	listener.setBlocking(false);
	safeSocket.setBlocking(false);
	fastSocket.setBlocking(false);
}

const sf::Socket::Status PacketHandler::Listen(const unsigned short port, const bool block)
{
	Socket::Status result = Socket::Status::NotReady;
	int error = 0;

	// First and foremost, we should bind the UDP socket.
	Bind(port, true);

	// If blocking is enabled, listen until a connection is established.
	result = listener.listen(port);

	// If there was an error, throw an exception.
	// If the result is otherwise non-critical and blocking is disabled, return its result.
	if (result == Socket::Status::Error)
		throw error = WSAGetLastError();
	else if (!block && result != Socket::Status::Done)
		return result;

	// Now attempt to accept the connection.
	do
	{
		result = listener.accept(safeSocket);
	} while (block && result == Socket::Status::NotReady);

	// Once again, throw an exception if necessary,
	// otherwise simply return the result.
	if (result == Socket::Status::Error)
		throw error = WSAGetLastError();
	else if (result != Socket::Status::Done)
		return result;

	// Pull the remote address out of the socket
	// and store it for later use with UDP.
	Address.ip = safeSocket.getRemoteAddress();

	// Set connection state to true to allow use of communication functions
	connected = true;
	// Set host state to true to ensure everything knows what to do.
	host = true;

	// The message ID to confirm we get the right packet.
	uchar ID = MSG_NULL;
	// Create a packet for receiving the local UDP port from the client.
	// Yeah, I'm doing it over TCP. Screw the rules.
	sf::Packet packet;

	// Wait for the packet, check the status, blah blah blah.
	// You get the idea at this point.
	if ((result = recvSafe(packet, true)) != Socket::Status::Done)
		return result;

	// Now to check the packet ID.
	// If it's not what we're looking for, I DON'T EVEN KNOW WHAT TO DO!
	// Start over I guess? Somebody help me out here.
	packet >> ID;
	if (ID != MSG_BIND)
	{
		ID = MSG_NULL;
		Disconnect(true);
		return Socket::Status::Disconnected;
	}

	// Now pull the port out of the packet.
	packet >> Address.port;

	// Set connection success time to be used by external functions.
	SetConnectTime();

	return result;
}
const sf::Socket::Status PacketHandler::Connect(sf::IpAddress ip, const unsigned short port, const bool block)
{
	Socket::Status result = Socket::Status::NotReady;
	int error = 0;
	if (!connected)
	{
		// First, let's bind the UDP port.
		// We're going to need its local port to send to the server.
		Bind(port, false);

		// Assign the address you klutz! STOP FORGETTING!
		Address.ip = ip;
		Address.port = port;

		// Now we attempt to connect to ip and port
		// If blocking is enabled, we wait until something happens.
		do
		{
			result = safeSocket.connect(ip, port);
		} while (block && result == Socket::Status::NotReady);

		// If there was an error, throw an exception.
		// If the result is otherwise non-critical and blocking is disabled, return its result.
		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();
		else if (!block && result != Socket::Status::Done)
			return result;

		// Set connection state to true to allow use of communication functions.
		connected = true;
		// Even though host defaults to false, ensure that it is in fact false.
		host = false;

		// The packet used to send the local UDP port.
		sf::Packet packet;

		// Now we populate the packet with the ID and the port,
		// and send it off, retreiving the status.
		packet << (uchar)MSG_BIND << fastSocket.getLocalPort();
		result = sendSafe(packet);

		// Same deal as last time...
		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();
		else if (result != Socket::Status::Done)
			return result;

		// Set connection success time to be used by external functions.
		SetConnectTime();

		sendFast(packet);
	}
	return result;
}
const sf::Socket::Status PacketHandler::Disconnect(const bool received)
{
	Socket::Status result = Socket::Status::Disconnected;

	if (!received && connected)
	{
		Packet disconnect;
		disconnect << (uchar)MSG_DISCONNECT;

		do
		{
			result = safeSocket.send(disconnect);
		} while (result == Socket::Status::NotReady);

		safeSocket.disconnect();
		fastSocket.unbind();
		listener.close();
	}

	connected = false;
	return result;
}
const sf::Socket::Status PacketHandler::Bind(const unsigned short port, const bool isServer)
{
	Socket::Status result = Socket::Status::NotReady;
	int error = 0;

	if ((result = fastSocket.bind((isServer) ? port : Socket::AnyPort)) != Socket::Status::Done)
	{
		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();
	}

	return result;
}

const sf::Socket::Status PacketHandler::Connect(RemoteAddress address, const bool block)
{
	return Connect(address.ip, address.port, block);
}

inline void PacketHandler::SetConnectTime()
{
	start_time = millisecs();
}

const sf::Socket::Status PacketHandler::Send(PacketEx& packet)
{
	if (!packet.isEmpty())
	{
		if (packet.isSafe)
			return sendSafe(packet);
		else
			return sendFast(packet);
	}

	return Socket::Status::NotReady;
}
const sf::Socket::Status PacketHandler::Receive(PacketEx& packet, const bool block)
{
	if (packet.isSafe)
		return recvSafe(packet, block);
	else
		return recvFast(packet, block);
}

const sf::Socket::Status PacketHandler::sendSafe(Packet& packet)
{
	Socket::Status result = Socket::Status::NotReady;
	if (connected)
	{
		int error = 0;

		safeLock.lock();

		do
		{
			result = safeSocket.send(packet);
		} while (result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();

		safeLock.unlock();
	}
	return result;
}
const sf::Socket::Status PacketHandler::recvSafe(Packet& packet, const bool block)
{
	Socket::Status result = Socket::Status::NotReady;
	if (connected)
	{
		int error = 0;

		safeLock.lock();

		do
		{
			result = safeSocket.receive(packet);
		} while (block && result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();

		safeLock.unlock();
	}
	return result;
}
const sf::Socket::Status PacketHandler::sendFast(Packet& packet)
{
	Socket::Status result = Socket::Status::NotReady;
	if (connected)
	{
		int error = 0;
		fastLock.lock();

		do
		{
			result = fastSocket.send(packet, Address.ip, Address.port);
		} while (result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();

		fastLock.unlock();
	}
	return result;
}
const sf::Socket::Status PacketHandler::recvFast(Packet& packet, const bool block)
{
	Socket::Status result = Socket::Status::NotReady;
	if (connected)
	{
		RemoteAddress recvaddr = {};
		int error = 0;

		fastLock.lock();

		do
		{
			result = fastSocket.receive(packet, recvaddr.ip, recvaddr.port);
		} while (block && result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();
		else if (recvaddr.ip != Address.ip)
			result = Socket::Status::NotReady;

		fastLock.unlock();
	}
	return result;
}

#pragma endregion