#include <SFML/Network.hpp>
#include <LazyTypedefs.h>

#include "PacketExtensions.h"

// Static variables
uint8 PacketEx::MessageTypeCount = 0xFF;
void PacketEx::SetMessageTypeCount(const uint8 msgCount) { MessageTypeCount = msgCount; }

PacketEx::PacketEx(const uint8 msgCount, const bool safe) : sf::Packet(), isSafe(safe), MessageTypes(nullptr)
{
	SetMessageTypeCount(msgCount);
	Initialize();
}
PacketEx::PacketEx(const bool safe) : isSafe(safe), MessageTypes(nullptr)
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
	
	if (MessageTypes != nullptr)
		delete[] MessageTypes;
	MessageTypes = new bool[MessageTypeCount]();
}

bool PacketEx::isInPacket(const uint8 type) const
{
	return MessageTypes[type];
}

bool PacketEx::addType(uint8 type)
{
	if (isInPacket(type))
	{
		return false;
	}
	else
	{
		empty = false;
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