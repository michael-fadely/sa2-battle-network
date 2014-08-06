#pragma once
#include <SFML\Network.hpp>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SA2ModLoader.h"

sf::Packet& operator <<(sf::Packet& packet, const Rotation& data)
{
	packet << data.x << data.y << data.z;
}

sf::Packet& operator <<(sf::Packet& packet, const Vertex& data)
{
	packet << data.x << data.y << data.z;
}