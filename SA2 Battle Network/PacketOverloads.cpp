#include "stdafx.h"

#include <SFML/Network/Packet.hpp>
#include <vector>

#include "PacketOverloads.h"

sf::Packet& operator <<(sf::Packet& packet, const char& data)
{
	return packet << (signed char)data;
}
sf::Packet& operator >>(sf::Packet& packet, char& data)
{
	return packet >> (signed char&)data;
}

// TODO: template
sf::Packet& operator<<(sf::Packet& packet, const std::vector<char>& data)
{
	packet << (sf::Uint32)data.size();
	packet.append(data.data(), data.size());
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, std::vector<char>& data)
{
	sf::Uint32 length;
	packet >> length;
	data.resize(length);

	for (sf::Uint32 i = 0; i < length; i++)
		packet >> data[i];

	return packet;
}
