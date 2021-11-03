#pragma once

#include <cstdint>
#include <sws/Packet.h>

struct sequence_t
{
	static const sequence_t dummy;
	uint16_t value;

	sequence_t();
	sequence_t(uint16_t value);

	bool operator==(sequence_t other) const;
	bool operator!=(sequence_t other) const;
	bool operator>(sequence_t  other) const;
	bool operator<(sequence_t  other) const;
	bool operator>=(sequence_t other) const;
	bool operator<=(sequence_t other) const;

	sequence_t& operator++();
	sequence_t& operator--();
	sequence_t  operator++(int);
	sequence_t  operator--(int);
};

template <>
struct std::hash<sequence_t>
{
	using argument_type = sequence_t;
	using result_type = std::size_t;

	result_type operator()(argument_type const& arg) const noexcept
	{
		return std::hash<uint16_t>()(arg.value);
	}
};

sws::Packet& operator<<(sws::Packet& packet, sequence_t data);
sws::Packet& operator>>(sws::Packet& packet, sequence_t& data);