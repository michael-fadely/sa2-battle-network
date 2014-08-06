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
	safeSocket.setBlocking(false);
	fastSocket.setBlocking(false);
}

const sf::Socket::Status PacketHandler::Bind(const unsigned short port, const bool isServer)
{
	Socket::Status result;
	int error = 0;

	do
	{
		result = fastSocket.bind((isServer) ? port : Socket::AnyPort);
	} while (result == Socket::Status::NotReady);

	if (result == Socket::Status::Error)
		throw error = WSAGetLastError();

	if (!isServer)
		Address.port = fastSocket.getLocalPort();
	return result;
}

const sf::Socket::Status PacketHandler::Listen(const unsigned short port)
{
	Socket::Status result;
	int error = 0;

	result = listener.listen(port);

	if (result == Socket::Status::Error)
		throw error = WSAGetLastError();
	else if (result != Socket::Status::Done)
		return result;

	result = listener.accept(safeSocket);
	if (result == Socket::Status::Error)
		throw error = WSAGetLastError();
	else if (result != Socket::Status::Done)
		return result;

	Address.ip = safeSocket.getRemoteAddress();
	Address.port = safeSocket.getRemotePort();

	Bind(port, true);

	host = true;
	connected = true;
	SetConnectTime();
	return result;
}
const sf::Socket::Status PacketHandler::Connect(RemoteAddress address)
{
	return Connect(address.ip, address.port);
}
const sf::Socket::Status PacketHandler::Connect(sf::IpAddress ip, const unsigned short port)
{
	Socket::Status result = Socket::Status::NotReady;
	int error = 0;
	if (!connected)
	{
		do
		{
			result = safeSocket.connect(ip, port);
		} while (result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();

		Bind(port, false);

		host = false;
		connected = true;
		SetConnectTime();
	}
	return result;
}
const sf::Socket::Status PacketHandler::Disconnect(const bool received)
{
	Socket::Status result = Socket::Status::Disconnected;

	if (connected)
	{
		Packet disconnect;
		disconnect << (uchar)MSG_DISCONNECT;

		do
		{
			result = safeSocket.send(disconnect);
		} while (result == Socket::Status::NotReady);
	}

	connected = false;
	return result;
}

inline void PacketHandler::SetConnectTime()
{
	start_time = millisecs();
}

const sf::Socket::Status PacketHandler::Send(PacketEx& packet)
{
	if (packet.isSafe)
		return sendSafe(packet);
	else
		return sendFast(packet);
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
		packet << (uchar)MSG_NULL;
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
		int error = 0;
		fastLock.lock();
		do
		{
			result = fastSocket.receive(packet, Address.ip, Address.port);
		} while (block && result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw error = WSAGetLastError();
		fastLock.unlock();
	}
	return result;
}

#pragma endregion