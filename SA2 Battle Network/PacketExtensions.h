#pragma once
#include <SFML/Network.hpp>
#include "typedefs.h"
#include "Networking.h"

class PacketEx : public sf::Packet
{
public:
	explicit PacketEx(const bool safe);
	~PacketEx();

	bool isInPacket(const nethax::MessageID type) const;

	bool isEmpty() const
	{
		return empty;
	}

	void Clear()
	{
		clear();
		Initialize();
	}

	// Returns the number of unique messages in this instance of the packet
	uint32 getMessageCount() const
	{
		return messageCount;
	}

	// Adds a message type to the packet.
	// Returns true if it was added, false if it already exists.
	bool addType(const nethax::MessageID type, bool allowDuplicates = false);

	// Determines whether or not the packet is "Safe" (TCP) or "Fast" (UDP)
	// This can be changed at any time before it is sent.
	bool isSafe;

private:
	void Initialize();

	bool empty;
	// Array of message types in the packet (true/false)
	bool* MessageTypes;
	// The number of messages currently in the packet
	uint32 messageCount;
	// UDP packet sequence number
	static ushort sequence;
};
