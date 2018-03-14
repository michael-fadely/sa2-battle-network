#include "stdafx.h"
#include "AdventurePacketOverloads.h"
#include "AddressList.h"

sws::Packet& operator <<(sws::Packet& packet, const Rotation& data)
{
	return packet << data.x << data.y << data.z;
}

sws::Packet& operator >>(sws::Packet& packet, Rotation& data)
{
	return packet >> data.x >> data.y >> data.z;
}

sws::Packet& operator <<(sws::Packet& packet, const NJS_VECTOR& data)
{
	return packet << data.x << data.y << data.z;
}

sws::Packet& operator >>(sws::Packet& packet, NJS_VECTOR& data)
{
	return packet >> data.x >> data.y >> data.z;
}

sws::Packet& operator <<(sws::Packet& packet, const PolarCoord& data)
{
	return packet << static_cast<int>(data.angle) << data.distance;
}

sws::Packet& operator >>(sws::Packet& packet, PolarCoord& data)
{
	return packet >> *reinterpret_cast<int*>(&data.angle) >> data.distance;
}
