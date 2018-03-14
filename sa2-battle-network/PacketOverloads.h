#pragma once

#include <sws/Packet.h>
#include <vector>

template <typename T>
sws::Packet& operator<<(sws::Packet& packet, const std::vector<T>& data)
{
	packet << static_cast<uint32_t>(data.size());
	packet.write_data(data.data(), sizeof(T) * data.size(), true);
	return packet;
}

template <typename T>
sws::Packet& operator>>(sws::Packet& packet, std::vector<T>& data)
{
	uint32_t length = 0;
	packet >> length;
	data.resize(length);
	packet.read_data(data.data(), sizeof(T) * length, true);
	return packet;
}
