#pragma once
#include <SA2ModLoader.h>
#include <sws/Packet.h>
#include "AddressList.h"

sws::Packet& operator <<(sws::Packet& packet, const Rotation& data);
sws::Packet& operator >>(sws::Packet& packet, Rotation& data);
sws::Packet& operator <<(sws::Packet& packet, const NJS_VECTOR& data);
sws::Packet& operator >>(sws::Packet& packet, NJS_VECTOR& data);
sws::Packet& operator >>(sws::Packet& packet, PolarCoord& data);
sws::Packet& operator <<(sws::Packet& packet, const PolarCoord& data);