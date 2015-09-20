#pragma once
#include <SFML/Network.hpp>
#include "typedefs.h"

class PacketEx : public sf::Packet
{
public:
	// Constructs a new instance, setting the global maximum message types to msgCount.
	PacketEx(const uint8 msgCount, const bool safe);
	// Constructs a new instance
	PacketEx(const bool safe);
	~PacketEx();

	bool isInPacket(const uint8 type) const;

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
	bool addType(uint8 type);

	// Determines whether or not the packet is "Safe" (TCP) or "Fast" (UDP)
	// This can be changed at any time before it is sent.
	bool isSafe;

	// Sets the global maximum message types for all packets.
	static void SetMessageTypeCount(const uint8 msgCount);

private:
	void Initialize();

	bool empty;
	// Array of message types in the packet (true/false)
	bool* MessageTypes;
	// The number of messages currently in the packet
	uint32 messageCount;
	// The global maximum number of message types handled by all packets.
	// This can be changed using SetMessageTypeCount
	static uint8 MessageTypeCount;
};

sf::Packet& operator <<(sf::Packet& packet, const char& data);
sf::Packet& operator >>(sf::Packet& packet, char& data);
