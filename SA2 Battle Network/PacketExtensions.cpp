#include "stdafx.h"

#include <SFML/Network.hpp>
#include <vector>
#include "Networking.h"
#include "typedefs.h"

#include "PacketExtensions.h"

using namespace nethax;

ushort PacketEx::sequence = 1;

PacketEx::PacketEx(nethax::Protocol protocol) : Packet(), Protocol(protocol), empty(true)
{
	initialize();
}

void PacketEx::initialize()
{
	if (Protocol != Protocol::TCP)
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

	messageTypes.clear();
	messageTypes.resize((int)MessageID::Count);
}

bool PacketEx::isInPacket(MessageID type) const
{
	return messageTypes[(int)type];
}

bool PacketEx::AddType(MessageID type, bool allowDupes)
{
	if (!allowDupes && isInPacket(type))
		return false;

	empty = false;
	*this << type;
	messageCount++;
	return messageTypes[(int)type] = true;
}
