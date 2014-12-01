#pragma once
#include <SFML\Network.hpp>
#include "SA2ModLoader.h"

sf::Packet& operator <<(sf::Packet& packet, const Rotation& data)
{
	return packet << data.x << data.y << data.z;
}

sf::Packet& operator <<(sf::Packet& packet, const Vertex& data)
{
	return packet << data.x << data.y << data.z;
}

sf::Packet& operator >>(sf::Packet& packet, Rotation& data)
{
	return packet >> data.x >> data.y >> data.z;
}

sf::Packet& operator >>(sf::Packet& packet, Vertex& data)
{
	return packet >> data.x >> data.y >> data.z;
}