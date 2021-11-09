#include "stdafx.h"

#include <chrono>
#include <thread>
#include <utility>
#include <stdexcept>

#include "reliable.h"
#include "ConnectionManager.h"
#include "Connection.h"

// TODO: ping
// TODO: disconnect

using namespace sws;
using namespace std::chrono;
using namespace reliable;

static constexpr auto AGE_THRESHOLD = 1s;

Connection::Store::Store(sequence_t sequence, Packet packet)
	: creation_time_(clock::now()),
	  last_active(clock::now()),
	  sequence(sequence),
	  packet(std::move(packet))
{
}

Connection::Store::Store(Store&& other) noexcept
{
	*this = std::move(other);
}

Connection::Store& Connection::Store::operator=(Store&& other) noexcept
{
	sequence       = other.sequence;
	packet         = std::move(other.packet);
	creation_time_ = other.creation_time_;
	last_active    = other.last_active;

	return *this;
}

Connection::clock::time_point Connection::Store::creation_time() const
{
	return creation_time_;
}

bool Connection::Store::should_send(const clock::duration& duration) const
{
	return clock::now() - last_active > duration;
}

void Connection::Store::reset_activity()
{
	last_active = clock::now();
}

Connection::Connection(ConnectionManager* parent_, std::shared_ptr<UdpSocket> socket_, Address remote_address_)
	: parent(parent_),
	  socket(std::move(socket_)),
	  remote_address_(std::move(remote_address_)),
	  is_connected_(true)
{
	for (auto& point : rtt_points)
	{
		point = 1s;
	}
}

Connection::Connection(Connection&& other) noexcept
{
	*this = std::move(other);
}

Connection& Connection::operator=(Connection&& other) noexcept
{
	parent = other.parent;
	socket = std::move(other.socket);

	inbound = std::move(other.inbound);

	remote_address_ = std::move(other.remote_address_);

	ordered_out = std::move(other.ordered_out);
	uids_out    = std::move(other.uids_out);
	acknew_data = std::move(other.acknew_data);

	seqs_in = std::move(other.seqs_in);
	uids_in = std::move(other.uids_in);

	rtt_points  = other.rtt_points;
	rtt_invalid = other.rtt_invalid;
	rtt_i       = other.rtt_i;
	current_rtt = other.current_rtt;

	return *this;
}

SocketState Connection::send(Packet& packet, bool block)
{
	if (!is_connected())
	{
		return SocketState::closed;
	}

	const auto read_pos = packet.tell(SeekCursor::read);
	const auto write_pos = packet.tell(SeekCursor::write);

	packet.seek(SeekCursor::both, SeekType::from_start, 0);

	reliable_t type = reliable_t::none;
	manage_id id = manage_id::eop;

	ptrdiff_t sequence_offset = -1;
	sequence_t outbound_sequence = 0;

	do
	{
		packet >> id;

		switch (id)  // NOLINT(clang-diagnostic-switch-enum)
		{
			case manage_id::eop:
				break;

			case manage_id::type:
				packet >> type;
				break;

			case manage_id::sequence:
			{
				sequence_offset = packet.tell(SeekCursor::read);

				sequence_t dummy_seq;
				packet >> dummy_seq;

				// to break out of the loop
				id = manage_id::eop;
				break;
			}

			default:
				throw;
		}
	} while (id != manage_id::eop);

	if (type == reliable_t::none)
	{
		if (sequence_offset != -1)
		{
			throw std::runtime_error("sequence specified in non-sequenced packet");
		}
	}
	else
	{
		if (sequence_offset == -1)
		{
			throw std::runtime_error("sequence offset was not reserved");
		}

		packet.seek(SeekCursor::write, SeekType::from_start, sequence_offset);

		switch (type)  // NOLINT(clang-diagnostic-switch-enum)
		{
			case reliable_t::newest:
				outbound_sequence = ++faf_out;
				packet << outbound_sequence;
				break;

			case reliable_t::ack:
				outbound_sequence = ++uid_out;
				packet << outbound_sequence;
				uids_out.emplace(outbound_sequence, Store(outbound_sequence, packet));
				break;

			case reliable_t::ack_newest:
				outbound_sequence = ++acknew_out;
				packet << outbound_sequence;
				acknew_data = std::make_unique<Store>(outbound_sequence, packet);
				break;

			case reliable_t::ordered:
				outbound_sequence = ++seq_out;
				packet << outbound_sequence;
				ordered_out.emplace_back(outbound_sequence, packet);
				break;

			default:
				throw;
		}
	}

	SocketState result = socket->send_to(packet, remote_address_);

	packet.seek(SeekCursor::read, SeekType::from_start, read_pos);
	packet.seek(SeekCursor::write, SeekType::from_start, write_pos);

	if (!block || result != SocketState::done)
	{
		return result;
	}

	switch (type)
	{
		case reliable_t::none:
		case reliable_t::newest:
			return result;

		case reliable_t::ack:
		{
			while (uids_out.find(outbound_sequence) != uids_out.end())
			{
				if ((result = parent->receive(true, 1)) == SocketState::error)
				{
					return result;
				}

				update();
				std::this_thread::sleep_for(1ms);
			}
			break;
		}

		case reliable_t::ack_newest:
		{
			while (acknew_data != nullptr)
			{
				if ((result = parent->receive(true, 1)) == SocketState::error)
				{
					return result;
				}

				update();
				std::this_thread::sleep_for(1ms);
			}

			break;
		}

		case reliable_t::ordered:
		{
			while (std::ranges::find_if(ordered_out,
			                            [&](const Store& s) { return s.sequence == outbound_sequence; }) != ordered_out.end())
			{
				if ((result = parent->receive(true, 1)) == SocketState::error)
				{
					return result;
				}

				update();
				std::this_thread::sleep_for(1ms);
			}
			break;
		}

		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw;
	}

	return result;
}

SocketState Connection::store_inbound(Packet& packet)
{
	if (!is_connected())
	{
		return SocketState::closed;
	}

	SocketState result = SocketState::done;

	reliable_t reliable_type = reliable_t::none;
	manage_id id = manage_id::eop;
	bool should_disconnect = false;

	sequence_t packet_sequence;

	do
	{
		packet >> id;

		if (id == manage_id::type)
		{
			if (reliable_type != reliable_t::none)
			{
				throw;
			}

			packet >> reliable_type;
			continue;
		}

		switch (id)  // NOLINT(clang-diagnostic-switch-enum)
		{
			case manage_id::eop:
				break;

			case manage_id::connect:
			{
				if (!should_disconnect)
				{
					Packet p;
					p << manage_id::connected << manage_id::eop;

					socket->send_to(p, remote_address_);

					id = manage_id::eop;
					result = SocketState::in_progress;
				}

				break;
			}

			case manage_id::disconnected:
				should_disconnect = true;
				break;

			case manage_id::connected:
			case manage_id::bad_version:
				return SocketState::in_progress;

			case manage_id::sequence:
				if (reliable_type == reliable_t::none)
				{
					throw;
				}

				packet >> packet_sequence;
				break;

			case manage_id::ack:
			{
				reliable_t type = reliable_t::none;
				sequence_t sequence;

				packet >> type >> sequence;

				remove_outbound(type, sequence);
				break;
			}

			default:
				throw;
		}
	} while (id != manage_id::eop);

	if (reliable_type != reliable_t::none && reliable_type != reliable_t::newest)
	{
		Packet p;

		p << manage_id::ack << reliable_type << packet_sequence << manage_id::eop;
		socket->send_to(p, remote_address_);

		if (handled(reliable_type, packet_sequence) && !should_disconnect)
		{
			return SocketState::in_progress;
		}
	}

	if (should_disconnect)
	{
		disconnect_internal();
		result = SocketState::closed;
	}
	else
	{
		inbound.emplace_back(std::move(packet));
	}

	return result;
}

bool Connection::handled(reliable_t type, sequence_t sequence)
{
	switch (type)
	{
		case reliable_t::none:
			return false;

		case reliable_t::newest:
			if (sequence <= faf_in)
			{
				return true;
			}

			faf_in = sequence;
			return false;

		case reliable_t::ack:
		{
			const auto it = uids_in.find(sequence);

			if (it != uids_in.end())
			{
				it->second = clock::now();
				return true;
			}

			uids_in[sequence] = clock::now();
			return false;
		}

		case reliable_t::ack_newest:
			if (sequence <= acknew_in)
			{
				return true;
			}

			acknew_in = sequence;
			return false;

		case reliable_t::ordered:
		{
			const auto it = seqs_in.find(sequence);

			if (it != seqs_in.end())
			{
				it->second = clock::now();
				return true;
			}

			seqs_in[sequence] = clock::now();
			return false;
		}

		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw;
	}
}

void Connection::disconnect_internal()
{
	inbound.clear();
	ordered_out.clear();
	uids_out.clear();
	seqs_in.clear();
	uids_in.clear();

	acknew_data = nullptr;

	is_connected_ = false;
}

void Connection::remove_outbound(reliable_t type, sequence_t sequence)
{
	switch (type)
	{
		case reliable_t::none:
			throw;

		case reliable_t::newest:
			return;

		case reliable_t::ack:
		{
			const auto it = uids_out.find(sequence);
			if (it != uids_out.end())
			{
				add_rtt_point(it->second.creation_time());
				uids_out.erase(it);
			}
			break;
		}

		case reliable_t::ack_newest:
			if (acknew_out == sequence && acknew_data != nullptr)
			{
				add_rtt_point(acknew_data->creation_time());
				acknew_data = nullptr;
			}
			break;

		case reliable_t::ordered:
		{
			const auto it = std::ranges::find_if(ordered_out, [sequence](const Store& s)
			{
				return s.sequence == sequence;
			});

			if (it != ordered_out.end())
			{
				add_rtt_point(it->creation_time());
				ordered_out.erase(it);
			}

			break;
		}

		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw;
	}
}

void Connection::prune()
{
	const auto now = clock::now();

	for (auto it = seqs_in.begin(); it != seqs_in.end();)
	{
		if (now - it->second >= AGE_THRESHOLD)
		{
			it = seqs_in.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = uids_in.begin(); it != uids_in.end();)
	{
		if (now - it->second >= AGE_THRESHOLD)
		{
			it = uids_in.erase(it);
		}
		else
		{
			++it;
		}
	}
}

Connection::clock::duration Connection::round_trip_time()
{
	if (rtt_invalid)
	{
		clock::duration::rep duration {};

		for (auto& point : rtt_points)
		{
			duration += point.count();
		}

		duration /= rtt_points.size();
		current_rtt = clock::duration(duration);
		rtt_invalid = false;
	}

	return current_rtt;
}

void Connection::update()
{
	prune();

	const clock::duration rtt = round_trip_time();

	if (!ordered_out.empty())
	{
		auto& store = ordered_out.front();

		if (store.should_send(rtt))
		{
			add_rtt_point(store.creation_time());
			socket->send_to(store.packet, remote_address_);
			store.reset_activity();
		}
	}

	for (auto& pair : uids_out)
	{
		auto& store = pair.second;

		if (store.should_send(rtt))
		{
			add_rtt_point(store.creation_time());
			socket->send_to(store.packet, remote_address_);
			store.reset_activity();
		}
	}

	if (acknew_data != nullptr)
	{
		if (acknew_data->should_send(rtt))
		{
			add_rtt_point(acknew_data->creation_time());
			socket->send_to(acknew_data->packet, remote_address_);
			acknew_data->reset_activity();
		}
	}
}

bool Connection::pop(sws::Packet* out_packet)
{
	if (inbound.empty())
	{
		return false;
	}

	auto packet = std::move(inbound.front());
	inbound.pop_front();

	if (out_packet)
	{
		*out_packet = std::move(packet);
	}

	return true;
}

bool Connection::is_connected() const
{
	return is_connected_;
}

const sws::Address& Connection::remote_address() const
{
	return remote_address_;
}

void Connection::disconnect()
{
	if (!is_connected())
	{
		return;
	}

	Packet packet;
	packet << manage_id::disconnected << manage_id::eop;

	socket->send_to(packet, remote_address_);

	disconnect_internal();
}

void Connection::add_rtt_point(const clock::time_point& point)
{
	rtt_points[rtt_i++] = clock::now() - point;
	rtt_i %= rtt_points.size();
	rtt_invalid = true;
}
