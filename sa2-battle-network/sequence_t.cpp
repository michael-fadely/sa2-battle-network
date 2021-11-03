#include "stdafx.h"
#include "sequence_t.h"

const sequence_t sequence_t::dummy(std::numeric_limits<uint16_t>::max());

sequence_t::sequence_t()
	: value(0)
{
}

sequence_t::sequence_t(uint16_t value)
	: value(value)
{
}

bool sequence_t::operator==(sequence_t other) const
{
	return this->value == other.value;
}

bool sequence_t::operator!=(sequence_t other) const
{
	return !(*this == other);
}

bool sequence_t::operator>(sequence_t other) const
{
	return (value > other.value && value - other.value <= 32768) ||
	       (value < other.value && other.value - value > 32768);
}

bool sequence_t::operator<(sequence_t other) const
{
	return other > *this;
}

bool sequence_t::operator>=(sequence_t other) const
{
	return *this == other || *this > other;
}

bool sequence_t::operator<=(sequence_t other) const
{
	return *this == other || *this < other;
}

sequence_t& sequence_t::operator++()
{
	++value;
	return *this;
}

sequence_t& sequence_t::operator--()
{
	--value;
	return *this;
}

sequence_t sequence_t::operator++(int)
{
	auto temp = *this;
	++(*this);
	return temp;
}

sequence_t sequence_t::operator--(int)
{
	auto temp = *this;
	--(*this);
	return temp;
}

sws::Packet& operator<<(sws::Packet& packet, sequence_t data)
{
	return packet << data.value;
}

sws::Packet& operator>>(sws::Packet& packet, sequence_t& data)
{
	return packet >> data.value;
}
