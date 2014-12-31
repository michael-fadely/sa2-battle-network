#include <SFML/Network.hpp>
#include <LazyTypedefs.h>

#include "PacketExtensions.h"

// Static variables
uint8 PacketEx::MessageTypeCount = 0xFF;
void PacketEx::SetMessageTypeCount(const uint8 msgCount) { MessageTypeCount = msgCount; }

PacketEx::PacketEx(const uint8 msgCount, const bool safe) : sf::Packet(), isSafe(safe)
{
	SetMessageTypeCount(msgCount);
	Initialize();
}
PacketEx::PacketEx(const bool safe) : isSafe(safe)
{
	Initialize();
}
PacketEx::~PacketEx()
{
	delete[] MessageTypes;
}

void PacketEx::Initialize()
{
	empty = true;
	messageCount = 0;
	MessageTypes = new bool[MessageTypeCount]();
}

const bool PacketEx::isInPacket(const uint8 type)
{
	return MessageTypes[type];
}

const bool PacketEx::addType(uint8 type)
{
	empty = false;
	if (isInPacket(type))
	{
		return false;
	}
	else
	{
		*this << type;
		messageCount++;
		return MessageTypes[type] = true;
	}
}


sf::Packet& operator <<(sf::Packet& packet, const char& data)
{
	return packet << (signed char)data;
}
sf::Packet& operator >>(sf::Packet& packet, char& data)
{
	return packet >> (signed char&)data;
}