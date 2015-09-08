#pragma once
#include <SFML/Network.hpp>
#include <SA2ModLoader.h>

sf::Packet& operator <<(sf::Packet& packet, const Rotation& data);
sf::Packet& operator <<(sf::Packet& packet, const Vertex& data);
sf::Packet& operator >>(sf::Packet& packet, Rotation& data);
sf::Packet& operator >>(sf::Packet& packet, Vertex& data);