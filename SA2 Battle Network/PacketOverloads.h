#pragma once
#include <SFML/Network.hpp>
#include "Networking.h"

sf::Packet& operator <<(sf::Packet& packet, const char& data);
sf::Packet& operator >>(sf::Packet& packet, char& data);

sf::Packet& operator <<(sf::Packet& packet, const std::vector<char>& data);
sf::Packet& operator >>(sf::Packet& packet, std::vector<char>& data);

sf::Packet& operator <<(sf::Packet& packet, const nethax::MessageID& data);
sf::Packet& operator >>(sf::Packet& packet, nethax::MessageID& data);
