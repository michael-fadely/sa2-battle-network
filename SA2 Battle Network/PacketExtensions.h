#pragma once
#include <SFML\Network.hpp>
#include <LazyTypedefs.h>

class PacketEx : public sf::Packet
{
public:
	// Construct a packet
	PacketEx(const uint8 msgCount, const bool safe);
	PacketEx(const bool safe);
	~PacketEx();

	const bool isInPacket(const uint8 type);
	const bool isEmpty() { return empty; }
	const unsigned int getMessageCount() { return messageCount; }

	// Adds a message type to the packet.
	// Returns true if it was added, false if it already exists.
	const bool addType(uint8 type);

	// Determines whether or not the packet is "Safe" (TCP) or "Fast" (UDP)
	const bool isSafe;

	// Static stuff
	// Set the global message count to be used by all packets
	static void SetMessageTypeCount(const uint8 msgCount);

private:
	void Initialize();

	bool empty;
	// Array of message types in the packet (true/false)
	bool* MessageTypes;
	// The number of messages currently in the packet
	unsigned int messageCount;
	// Static stuff some more
	// The number of message types handled by all packets.
	// This can be changed using SetMessageCount
	static uint8 MessageTypeCount;
};

sf::Packet& operator <<(sf::Packet& packet, const char& data);
sf::Packet& operator >>(sf::Packet& packet, char& data);