#include "PacketExtensions.h"

// Static variables
unsigned char PacketExt::MessageTypeCount = 0xFF;
void PacketExt::SetMessageTypeCount(const unsigned char msgCount) { MessageTypeCount = msgCount; }

PacketExt::PacketExt(const unsigned char msgCount, const bool safe) : isSafe(safe)
{
	SetMessageTypeCount(msgCount);
	Initialize();
}
PacketExt::PacketExt(const bool safe) : isSafe(safe)
{
	Initialize();
}
PacketExt::~PacketExt()
{
	delete[] MessageTypes;
}

void PacketExt::Initialize()
{
	empty = true;
	messageCount = 0;
	MessageTypes = new bool[MessageTypeCount]();
}

const bool PacketExt::isInPacket(const unsigned char type)
{
	return MessageTypes[type];
}

const bool PacketExt::addType(unsigned char type)
{
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
