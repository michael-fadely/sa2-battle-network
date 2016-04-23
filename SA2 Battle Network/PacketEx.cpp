#include "stdafx.h"

#include <SFML/Network.hpp>
#include <vector>
#include "Networking.h"
#include "typedefs.h"

#include "PacketEx.h"

using namespace nethax;

ushort PacketEx::sequence = 1;

PacketEx::PacketEx(nethax::Protocol protocol) : Packet(), Protocol(protocol), empty(true),
building(false), sizeOffset(0), dataStart(0)
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
	building = false;
	messageCount = 0;

	messageTypes.clear();
	messageTypes.resize((int)MessageID::Count);
}

bool PacketEx::isInPacket(MessageID type) const
{
	return messageTypes[(int)type];
}

void PacketEx::Clear()
{
	clear();
	initialize();
}

size_t PacketEx::GetTypeSize() const
{
	if (!building)
		return 0;

	return posWrite() - dataStart;
}

bool PacketEx::AddType(MessageID type, bool allowDupes)
{
	if (!allowDupes && isInPacket(type))
		return false;

	if (building)
		throw;

	empty = false;
	building = true;

	*this << type;
	sizeOffset = posWrite();
	*this << static_cast<ushort>(0xFFFF);
	dataStart = posWrite();

	messageCount++;
	return messageTypes[(int)type] = true;
}

void PacketEx::Finalize()
{
	if (!building)
		throw;

	auto position = posWrite();
	auto size = position - dataStart;
	seekWrite(sizeOffset, SEEK_SET);
	*this << static_cast<ushort>(size);
	seekWrite(position, SEEK_SET);

	sizeOffset = 0;
	dataStart = 0;
	building = false;
}
