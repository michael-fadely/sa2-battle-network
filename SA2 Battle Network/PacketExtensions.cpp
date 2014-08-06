#include "PacketExtensions.h"

// Static variables
unsigned char PacketEx::MessageTypeCount = 0xFF;
void PacketEx::SetMessageTypeCount(const unsigned char msgCount) { MessageTypeCount = msgCount; }

PacketEx::PacketEx(const unsigned char msgCount, const bool safe) : sf::Packet(), isSafe(safe)
{
	SetMessageTypeCount(msgCount);
	Initialize();
}
PacketEx::PacketEx(const bool safe) : isSafe(safe)
{
	Initialize();
}
PacketEx::~PacketEx()
{
	delete[] MessageTypes;
}

void PacketEx::Initialize()
{
	empty = true;
	messageCount = 0;
	MessageTypes = new bool[MessageTypeCount]();
}

const bool PacketEx::isInPacket(const unsigned char type)
{
	return MessageTypes[type];
}

const bool PacketEx::addType(unsigned char type)
{
	empty = false;
	if (isInPacket(type))
	{
		return false;
	}
	else
	{
		*this << type;
		messageCount++;
		return MessageTypes[type] = true;
	}
}
