#include "stdafx.h"

#include <vector>
#include "Networking.h"
#include "typedefs.h"

#include "PacketEx.h"

using namespace nethax;

PacketEx::PacketEx(PacketChannel channel)
	: channel(channel),
	  empty_(true),
	  building(false),
	  size_offset(0),
	  data_start(0)
{
	initialize();
}

void PacketEx::initialize()
{
	if (channel == PacketChannel::reliable)
	{
		reliable::reserve(*this, reliable::reliable_t::ordered);
	}
	else
	{
		reliable::reserve(*this, reliable::reliable_t::newest);
	}

	empty_ = true;
	building = false;
	message_count_ = 0;

	types.clear();
	types.resize(static_cast<int>(MessageID::Count));
}

bool PacketEx::contains(MessageID type) const
{
	return types[static_cast<int>(type)];
}

bool PacketEx::is_empty() const
{
	return empty_;
}

void PacketEx::clear()
{
	Packet::clear();
	initialize();
}

uint PacketEx::message_count() const
{
	return message_count_;
}

size_t PacketEx::get_type_size() const
{
	if (!building)
	{
		return 0;
	}

	return tell(sws::SeekCursor::write) - data_start;
}

bool PacketEx::add_type(MessageID type, bool allow_dupes)
{
	if (!allow_dupes && contains(type))
	{
		return false;
	}

	if (building)
	{
		throw;
	}

	empty_ = false;
	building = true;

	*this << type;
	size_offset = tell(sws::SeekCursor::write);
	*this << static_cast<ushort>(0xFFFF);
	data_start = tell(sws::SeekCursor::write);

	++message_count_;
	return types[static_cast<int>(type)] = true;
}

void PacketEx::finalize()
{
	if (!building)
	{
		throw;
	}

	const ptrdiff_t position = tell(sws::SeekCursor::write);
	const size_t size = position - data_start;

	seek(sws::SeekCursor::write, sws::SeekType::from_start, size_offset);

	*this << static_cast<ushort>(size);

	seek(sws::SeekCursor::write, sws::SeekType::from_start, position);

	size_offset = 0;
	data_start = 0;
	building = false;
}
