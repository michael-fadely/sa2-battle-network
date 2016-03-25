#include "stdafx.h"

#include <SFML/Network/Packet.hpp>

#include "PacketOverloads.h"

sf::Packet& operator <<(sf::Packet& packet, const char& data)
{
	return packet << (signed char)data;
}
sf::Packet& operator >>(sf::Packet& packet, char& data)
{
	return packet >> (signed char&)data;
}
