#pragma once

#include <SFML/Network/Packet.hpp>
#include <vector>

sf::Packet& operator <<(sf::Packet& packet, const char& data);
sf::Packet& operator >>(sf::Packet& packet, char& data);

template <typename T>
sf::Packet& operator<<(sf::Packet& packet, const std::vector<T>& data)
{
	packet << (sf::Uint32)data.size();
	for (sf::Uint32 i = 0; i < data.size(); i++)
		packet << data[i];
	return packet;
}

template <typename T>
sf::Packet& operator>>(sf::Packet& packet, std::vector<T>& data)
{
	sf::Uint32 length;
	packet >> length;
	data.resize(length);

	for (sf::Uint32 i = 0; i < length; i++)
		packet >> data[i];

	return packet;
}
