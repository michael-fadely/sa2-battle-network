#pragma once

#include <cstdint>
#include <sws/Packet.h>

namespace reliable
{
	enum class manage_id : uint8_t
	{
		eop          = 127, // end of packet
		connect      = 'C', // connect request
		disconnected = 'D', // disconnect notification
		connected    = 'O', // O as in 'opened' I guess
		type         = 'T', // reliable type (reliable_t)
		sequence     = 'S', // sequence number (sequence_t)
		ack          = 'A',
		bad_version  = 'V',
	};

	enum class reliable_t : uint8_t
	{
		/**
		 * \brief No special treatment required.
		 */
		none,

		/**
		 * \brief No ack required, but check the sequence number.
		 */
		newest,

		/**
		 * \brief Receiver should acknowledge once received;
		 * no other special treatment required.
		 */
		ack,

		/**
		 * \brief Receiver should store only if newer than last received.
		 */
		ack_newest,

		/**
		 * \brief Same as regular ack, but the client holds back
		 * until an ack is received back
		 */
		ordered,
	};

	void reserve(sws::Packet& packet, reliable_t type);
	sws::Packet reserve(reliable_t type);
}

sws::Packet& operator <<(sws::Packet& packet, reliable::manage_id   data);
sws::Packet& operator >>(sws::Packet& packet, reliable::manage_id&  data);
sws::Packet& operator <<(sws::Packet& packet, reliable::reliable_t  data);
sws::Packet& operator >>(sws::Packet& packet, reliable::reliable_t& data);
