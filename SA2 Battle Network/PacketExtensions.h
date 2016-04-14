#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include "typedefs.h"
#include "Networking.h"

class PacketEx : public sf::Packet
{
public:
	explicit PacketEx(nethax::Protocol protocol);

	bool isInPacket(nethax::MessageID type) const;

	bool isEmpty() const
	{
		return empty;
	}

	void Clear()
	{
		clear();
		initialize();
	}

	// Returns the number of unique messages in this instance of the packet
	uint32 GetMessageCount() const
	{
		return messageCount;
	}

	// Adds a message type to the packet.
	// Returns true if it was added, false if it already exists.
	bool AddType(nethax::MessageID type, bool allowDupes = false);

	// Determines whether or not the packet is "Safe" (TCP) or "Fast" (UDP)
	// This can be changed at any time before it is sent.
	nethax::Protocol Protocol;

private:
	void initialize();

	bool empty;
	// Array of message types in the packet (true/false)
	std::vector<bool> messageTypes;
	// The number of messages currently in the packet
	uint32 messageCount;
	// UDP packet sequence number
	static ushort sequence;
};
