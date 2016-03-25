#pragma once
#include <SFML/Network.hpp>
#include <SA2ModLoader.h>
#include "AddressList.h"

sf::Packet& operator <<(sf::Packet& packet, const Rotation& data);
sf::Packet& operator >>(sf::Packet& packet, Rotation& data);
sf::Packet& operator <<(sf::Packet& packet, const NJS_VECTOR& data);
sf::Packet& operator >>(sf::Packet& packet, NJS_VECTOR& data);
sf::Packet& operator >>(sf::Packet& packet, PolarCoord& data);
sf::Packet& operator <<(sf::Packet& packet, const PolarCoord& data);