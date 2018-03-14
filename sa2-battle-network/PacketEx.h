#pragma once

#include <vector>
#include <sws/Packet.h>

#include "typedefs.h"
#include "Networking.h"

class PacketEx : public sws::Packet
{
public:
	explicit PacketEx(nethax::Protocol protocol);

	bool contains(nethax::MessageID type) const;

	bool is_empty() const;

	void clear() override;

	// Returns the number of unique messages in this instance of the packet
	uint message_count() const;

	size_t get_type_size() const;

	// Adds a message type to the packet.
	// Returns true if it was added, false if it already exists.
	bool add_type(nethax::MessageID type, bool allow_dupes = false);
	void finalize();

	// Determines whether or not the packet is "Safe" (TCP) or "Fast" (UDP)
	// This can be changed at any time before it is sent.
	nethax::Protocol protocol;

private:
	void initialize();

	bool empty_;
	bool building;
	size_t size_offset, data_start;

	// Array of message types in the packet (true/false)
	std::vector<bool> types;
	// The number of messages currently in the packet
	uint message_count_ {};
	// UDP packet sequence number
	static ushort sequence;
};
