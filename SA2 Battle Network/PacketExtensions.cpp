#include "stdafx.h"

#include <SFML/Network.hpp>
#include "Networking.h"
#include "typedefs.h"

#include "PacketExtensions.h"

ushort PacketEx::sequence = 1;

PacketEx::PacketEx(const bool safe) : sf::Packet(), isSafe(safe), empty(true), messageTypes(nullptr)
{
	initialize();
}
PacketEx::~PacketEx()
{
	delete[] messageTypes;
}

void PacketEx::initialize()
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

	if (messageTypes != nullptr)
		delete[] messageTypes;

	messageTypes = new bool[(int)nethax::MessageID::Count]();
}

bool PacketEx::isInPacket(const nethax::MessageID type) const
{
	return messageTypes[(int)type];
}

bool PacketEx::AddType(const nethax::MessageID type, bool allowDuplicates)
{
	if (!allowDuplicates && isInPacket(type))
		return false;

	empty = false;
	*this << type;
	messageCount++;
	return messageTypes[(int)type] = true;
}
