#pragma once

#include <SFML/Network/Packet.hpp>
#include <vector>

sf::Packet& operator <<(sf::Packet& packet, const char& data);
sf::Packet& operator >>(sf::Packet& packet, char& data);

sf::Packet& operator <<(sf::Packet& packet, const std::vector<char>& data);
sf::Packet& operator >>(sf::Packet& packet, std::vector<char>& data);

