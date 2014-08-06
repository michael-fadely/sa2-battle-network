#include <iostream>
#include "Common.h"		// millisecs(), LazyTypedefs
#include "Networking.h"
#include "PacketHandler.h"

using namespace std;
using namespace sf;

PacketHandler::PacketHandler() : start_time(0), host(false), connected(false), Address({})
{
	Initialize();
}
PacketHandler::~PacketHandler()
{
	if (connected)
		Disconnect();
}
void PacketHandler::Initialize()
{
	cout << "<> Initializing packet handler..." << endl;
	//recvTimer = sendTimer = 0;
	safeSocket.setBlocking(false);
	fastSocket.setBlocking(false);
}

const Socket::Status PacketHandler::Listen(const unsigned short port)
{
	Socket::Status result;

	result = listener.listen(port);

	if (result == Socket::Status::Error)
		throw;
	else if (result != Socket::Status::Done)
		return result;

	result = listener.accept(safeSocket);
	if (result == Socket::Status::Error)
		throw;
	else if (result != Socket::Status::Done)
		return result;

	host = true;
	connected = true;
	SetConnectTime();
	return result;
}
const sf::Socket::Status PacketHandler::Connect(RemoteAddress address)
{
	return Connect(address.ip, address.port);
}
const Socket::Status PacketHandler::Connect(sf::IpAddress ip, const unsigned short port)
{
	Socket::Status result = Socket::Status::NotReady;
	if (!connected)
	{

		do
		{
			result = safeSocket.connect(ip, port);
		} while (result == Socket::Status::NotReady);

		if (result == Socket::Status::Error)
			throw;

		host = false;
		connected = true;
		SetConnectTime();
	}
	return result;
}
const Socket::Status PacketHandler::Disconnect()
{
	Socket::Status result = Socket::Status::NotReady;

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

const Socket::Status PacketHandler::sendSafe(Packet& packet)
{
	packet << (uchar)MSG_NULL;
	Socket::Status result;
	safeLock.lock();
	do
	{
		result = safeSocket.send(packet);
	} while (result == Socket::Status::NotReady);

	if (result == Socket::Status::Error)
		throw;
	safeLock.unlock();
	return result;
}
const Socket::Status PacketHandler::recvSafe(Packet& packet, const bool block)
{
	Socket::Status result;
	safeLock.lock();
	do
	{
		result = safeSocket.receive(packet);
	} while (block && result == Socket::Status::NotReady);

	if (result == Socket::Status::Error)
		throw;
	safeLock.unlock();
	return result;
}
const Socket::Status PacketHandler::sendFast(Packet& packet)
{
	packet << (uchar)MSG_NULL;
	Socket::Status result;
	fastLock.lock();
	do
	{
		result = fastSocket.send(packet, Address.ip, Address.port);
	} while (result == Socket::Status::NotReady);

	if (result == Socket::Status::Error)
		throw;
	fastLock.unlock();
	return result;
}
const Socket::Status PacketHandler::recvFast(Packet& packet, const bool block)
{
	Socket::Status result;
	fastLock.lock();
	do
	{
		result = fastSocket.receive(packet, Address.ip, Address.port);
	} while (block && result == Socket::Status::NotReady);

	if (result == Socket::Status::Error)
		throw;
	fastLock.unlock();
	return result;
}

