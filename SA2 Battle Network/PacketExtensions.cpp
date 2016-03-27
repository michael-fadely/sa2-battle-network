#include "stdafx.h"

#include <SFML/Network.hpp>
#include "Networking.h"
#include "typedefs.h"

#include "PacketExtensions.h"

ushort PacketEx::sequence = 1;

PacketEx::PacketEx(const bool safe) : sf::Packet(), isSafe(safe), empty(true), MessageTypes(nullptr)
{
	Initialize();
}
PacketEx::~PacketEx()
{
	delete[] MessageTypes;
}

void PacketEx::Initialize()
{
	if (!isSafe)
	{
		if (!empty)
		{
			sequence %= USHRT_MAX;
			++sequence;
		}

		*this << sequence;
	}

	empty = true;
	messageCount = 0;

	if (MessageTypes != nullptr)
		delete[] MessageTypes;

	MessageTypes = new bool[(int)nethax::MessageID::Count]();
}

bool PacketEx::isInPacket(const nethax::MessageID type) const
{
	return MessageTypes[(int)type];
}

bool PacketEx::addType(const nethax::MessageID type, bool allowDuplicates)
{
	if (!allowDuplicates && isInPacket(type))
		return false;

	empty = false;
	*this << type;
	messageCount++;
	return MessageTypes[(int)type] = true;
}

sf::Packet& operator <<(sf::Packet& packet, const char& data)
{
	return packet << (signed char)data;
}
sf::Packet& operator >>(sf::Packet& packet, char& data)
{
	return packet >> (signed char&)data;
}

sf::Packet& operator<<(sf::Packet& packet, const nethax::MessageID& data)
{
	return packet << (uint8)data;
}

sf::Packet& operator>>(sf::Packet& packet, nethax::MessageID& data)
{
	uint8 d;
	packet >> d; data = (nethax::MessageID)d;
	return packet;
}
