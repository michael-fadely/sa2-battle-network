#include <iostream>

#include "Common.h"
#include "Application.h"
#include "MemoryHandler.h"
#include "Networking.h"

#include "PacketHandler.h"

using namespace std;
using namespace chrono;

PacketHandler::PacketHandler() : connected(false)
{
	Initialize();

}

PacketHandler::~PacketHandler()
{
}

void PacketHandler::Initialize()
{
	safeSocket.setBlocking(false);
	fastSocket.setBlocking(false);
}

const sf::Socket::Status PacketHandler::Connect()
{
	sf::Socket::Status result = listener.listen(Address.port);

	if (result == sf::Socket::Status::Error)
		throw;
	else if (result != sf::Socket::Done)
		return result;

	result = listener.accept(safeSocket);
	if (result == sf::Socket::Error)
		throw;
	else if (result != sf::Socket::Done)
		return result;

	return result;
}

const sf::Socket::Status PacketHandler::sendSafe(sf::Packet& packet)
{
	packet << (uchar)MSG_NULL;
	sf::Socket::Status result;
	do
	{
		result = safeSocket.send(packet);
	} while (result == sf::Socket::NotReady);

	if (result == sf::Socket::Error)
		throw;

	return result;
}
const sf::Socket::Status PacketHandler::recvSafe(sf::Packet& packet, const bool block)
{
	sf::Socket::Status result;
	do
	{
		result = safeSocket.receive(packet);
	} while (result == sf::Socket::NotReady);

	if (result == sf::Socket::Error)
		throw;

	return result;
}
const sf::Socket::Status PacketHandler::sendFast(sf::Packet& packet)
{
	packet << (uchar)MSG_NULL;
	sf::Socket::Status result;
	do
	{
		result = fastSocket.send(packet, Address.ip, Address.port);
	} while (result == sf::Socket::NotReady);

	if (result == sf::Socket::Error)
		throw;

	return result;
}
const sf::Socket::Status PacketHandler::recvFast(sf::Packet& packet, const bool block)
{
	sf::Socket::Status result;
	do
	{
		result = fastSocket.receive(packet, Address.ip, Address.port);
	} while (result == sf::Socket::NotReady);

	if (result == sf::Socket::Error)
		throw;

	return result;
}

