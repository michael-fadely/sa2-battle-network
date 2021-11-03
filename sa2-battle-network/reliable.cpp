#include "stdafx.h"
#include <sws/Packet.h>
#include "reliable.h"
#include "sequence_t.h"

using namespace sws;

static_assert(sizeof(reliable::manage_id) == sizeof(uint8_t), "nope");
static_assert(sizeof(reliable::reliable_t) == sizeof(uint8_t), "nope");

Packet& operator <<(Packet& packet, reliable::manage_id data)
{
	return packet << static_cast<uint8_t>(data);
}

Packet& operator >>(Packet& packet, reliable::manage_id& data)
{
	return packet >> *reinterpret_cast<uint8_t*>(&data);
}

Packet& operator <<(Packet& packet, reliable::reliable_t data)
{
	return packet << static_cast<uint8_t>(data);
}

Packet& operator >>(Packet& packet, reliable::reliable_t& data)
{
	return packet >> *reinterpret_cast<uint8_t*>(&data);
}

void reliable::reserve(Packet& packet, reliable_t type)
{
	packet.seek(SeekCursor::write, SeekType::from_start, 0);
	packet << manage_id::type << type;

	if (type == reliable_t::none)
	{
		packet << manage_id::eop;
		return;
	}

	packet << manage_id::sequence;

	switch (type)
	{
		case reliable_t::newest:
		case reliable_t::ack:
		case reliable_t::ack_newest:
		case reliable_t::ordered:
			break;

		default:
			throw;
	}

	packet << sequence_t::dummy << manage_id::eop;
}

Packet reliable::reserve(reliable_t type)
{
	Packet packet;
	reserve(packet, type);
	return packet;
}
