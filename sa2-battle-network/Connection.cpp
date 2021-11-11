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

static constexpr auto age_threshold = 2.5s;

Connection::Store::Store(sequence_t sequence, Packet packet)
	: creation_time_(clock::now()),
	  last_active_(clock::now()),
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
	last_active_   = other.last_active_;

	return *this;
}

Connection::clock::time_point Connection::Store::creation_time() const
{
	return creation_time_;
}

bool Connection::Store::should_send(const clock::duration& duration) const
{
	return clock::now() - last_active_ > duration;
}

void Connection::Store::reset_activity()
{
	last_active_ = clock::now();
}

Connection::Connection(ConnectionManager* parent, std::shared_ptr<UdpSocket> socket, Address remote_address)
	: parent_(parent),
	  socket_(std::move(socket)),
	  remote_address_(std::move(remote_address)),
	  is_connected_(true)
{
	for (auto& point : rtt_points_)
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
	parent_ = other.parent_;
	socket_ = std::move(other.socket_);

	inbound_ = std::move(other.inbound_);

	remote_address_ = std::move(other.remote_address_);

	ordered_out_ = std::move(other.ordered_out_);
	uids_out_    = std::move(other.uids_out_);
	acknew_data_ = std::move(other.acknew_data_);

	ordered_in_ = std::move(other.ordered_in_);
	uids_in_    = std::move(other.uids_in_);

	rtt_points_  = other.rtt_points_;
	rtt_invalid_ = other.rtt_invalid_;
	rtt_i_       = other.rtt_i_;
	current_rtt_ = other.current_rtt_;

	is_connected_ = std::exchange(other.is_connected_, false);

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

	reliable_t reliable_type = reliable_t::none;
	manage_id id = manage_id::eop;

	ptrdiff_t sequence_offset = -1;

	do
	{
		packet >> id;

		switch (id)  // NOLINT(clang-diagnostic-switch-enum)
		{
			case manage_id::eop:
				break;

			case manage_id::type:
				packet >> reliable_type;
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
				throw std::runtime_error("malformed packet header");
		}
	} while (id != manage_id::eop);

	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	bool should_send = false;

	sequence_t outbound_sequence;

	if (reliable_type == reliable_t::none)
	{
		if (sequence_offset != -1)
		{
			throw std::runtime_error("sequence specified in non-sequenced packet");
		}

		should_send = true;
	}
	else
	{
		if (sequence_offset == -1)
		{
			throw std::runtime_error("sequence offset was not reserved");
		}

		packet.seek(SeekCursor::write, SeekType::from_start, sequence_offset);

		switch (reliable_type)  // NOLINT(clang-diagnostic-switch-enum)
		{
			case reliable_t::take_newest:
				should_send = true;
				outbound_sequence = ++faf_out_;
				packet << outbound_sequence;
				break;

			case reliable_t::ack_any:
				should_send = true;
				outbound_sequence = ++uid_out_;
				packet << outbound_sequence;
				uids_out_.emplace(outbound_sequence, Store(outbound_sequence, packet));
				break;

			case reliable_t::ack_newest:
				should_send = true;
				outbound_sequence = ++acknew_out_;
				packet << outbound_sequence;
				acknew_data_ = std::make_unique<Store>(outbound_sequence, packet);
				break;

			case reliable_t::ack_ordered:
				// only send immediately if there are no other
				// ordered packets waiting to be ack'd.
				should_send = ordered_out_.empty();
				outbound_sequence = ++seq_out_;
				packet << outbound_sequence;
				ordered_out_.emplace_back(outbound_sequence, packet);
				break;

			default:
				throw std::runtime_error("invalid reliable type");
		}
	}

	SocketState result = SocketState::in_progress;

	if (should_send)
	{
		result = socket_->send_to(packet, remote_address_);
	}

	packet.seek(SeekCursor::read, SeekType::from_start, read_pos);
	packet.seek(SeekCursor::write, SeekType::from_start, write_pos);

	if (!block || result == SocketState::error || result == SocketState::closed)
	{
		return result;
	}

	auto do_garbage = [&]() -> bool
	{
		update_outbound();
		result = parent_->receive(false, 1);

		if (result == SocketState::error)
		{
			return false;
		}

		if (result == SocketState::in_progress)
		{
			std::this_thread::sleep_for(1ms);
		}

		return true;
	};

	switch (reliable_type)
	{
		case reliable_t::none:
		case reliable_t::take_newest:
			return result;

		case reliable_t::ack_any:
		{
			while (uids_out_.contains(outbound_sequence))
			{
				if (!do_garbage())
				{
					return result;
				}
			}
			break;
		}

		case reliable_t::ack_newest:
		{
			while (acknew_data_ != nullptr)
			{
				if (!do_garbage())
				{
					return result;
				}
			}

			break;
		}

		case reliable_t::ack_ordered:
		{
			while (std::ranges::find_if(ordered_out_,
			                            [&](const Store& s) { return s.sequence == outbound_sequence; }) != ordered_out_.end())
			{
				if (!do_garbage())
				{
					return result;
				}
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
	bool should_store = true;

	sequence_t packet_sequence;

	do
	{
		packet >> id;

		if (id == manage_id::type)
		{
			if (reliable_type != reliable_t::none)
			{
				throw std::runtime_error("reliable type was specified twice in inbound packet");
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

					socket_->send_to(p, remote_address_);

					id = manage_id::eop;
					result = SocketState::in_progress;
					should_store = false;
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
					throw std::runtime_error("received packet sequence when none should be present!");
				}

				packet >> packet_sequence;
				break;

			case manage_id::ack:
			{
				should_store = false;

				reliable_t type = reliable_t::none;
				sequence_t sequence;

				packet >> type >> sequence;

				remove_outbound(type, sequence);
				break;
			}

			default:
				throw std::runtime_error("unhandled manage_id");
		}
	} while (id != manage_id::eop);

	if (reliable_type != reliable_t::none)
	{
		if (reliable_type != reliable_t::take_newest)
		{
			Packet p;

			p << manage_id::ack << reliable_type << packet_sequence << manage_id::eop;
			socket_->send_to(p, remote_address_);
		}

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
	else if (should_store)
	{
		inbound_.emplace_back(std::move(packet));
	}

	return result;
}

bool Connection::handled(reliable_t type, sequence_t sequence)
{
	switch (type)
	{
		case reliable_t::none:
			return false;

		case reliable_t::take_newest:
			if (sequence <= faf_in_)
			{
				return true;
			}

			faf_in_ = sequence;
			return false;

		case reliable_t::ack_any:
		{
			const auto it = uids_in_.find(sequence);

			if (it != uids_in_.end())
			{
				it->second = clock::now();
				return true;
			}

			uids_in_[sequence] = clock::now();
			return false;
		}

		case reliable_t::ack_newest:
			if (sequence <= ack_newest_in_)
			{
				return true;
			}

			ack_newest_in_ = sequence;
			return false;

		case reliable_t::ack_ordered:
		{
			// FIXME: what if ordered packets are sent faster than they can be pruned? sequence IDs will overlap when the data is actually unique!
			const auto it = ordered_in_.find(sequence);

			if (it != ordered_in_.end())
			{
				it->second = clock::now();
				return true;
			}

			ordered_in_[sequence] = clock::now();
			return false;
		}

		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw;
	}
}

void Connection::disconnect_internal()
{
	inbound_.clear();
	ordered_out_.clear();
	uids_out_.clear();
	ordered_in_.clear();
	uids_in_.clear();

	acknew_data_ = nullptr;

	is_connected_ = false;
}

void Connection::remove_outbound(reliable_t reliable_type, sequence_t sequence)
{
	switch (reliable_type)
	{
		case reliable_t::none:
			throw;

		case reliable_t::take_newest:
			return;

		case reliable_t::ack_any:
		{
			const auto it = uids_out_.find(sequence);

			if (it != uids_out_.end())
			{
				add_rtt_point(it->second.creation_time());
				uids_out_.erase(it);
			}

			break;
		}

		case reliable_t::ack_newest:
			if (acknew_out_ == sequence && acknew_data_ != nullptr)
			{
				add_rtt_point(acknew_data_->creation_time());
				acknew_data_ = nullptr;
			}
			break;

		case reliable_t::ack_ordered:
		{
			const auto it = std::ranges::find_if(ordered_out_, [sequence](const Store& s)
			{
				return s.sequence == sequence;
			});

			if (it != ordered_out_.end())
			{
				add_rtt_point(it->creation_time());
				ordered_out_.erase(it);
			}

			break;
		}

		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw;
	}
}

void Connection::prune_inbound_ids()
{
	const auto now = clock::now();

	for (auto it = ordered_in_.begin(); it != ordered_in_.end();)
	{
		if (now - it->second >= age_threshold)
		{
			it = ordered_in_.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = uids_in_.begin(); it != uids_in_.end();)
	{
		if (now - it->second >= age_threshold)
		{
			it = uids_in_.erase(it);
		}
		else
		{
			++it;
		}
	}
}

Connection::clock::duration Connection::round_trip_time()
{
	if (rtt_invalid_)
	{
		clock::duration::rep duration {};

		for (auto& point : rtt_points_)
		{
			duration += point.count();
		}

		duration /= rtt_points_.size();
		current_rtt_ = clock::duration(duration);
		rtt_invalid_ = false;
	}

	return current_rtt_;
}

void Connection::update_outbound()
{
	prune_inbound_ids();

	const clock::duration rtt = round_trip_time();

	if (!ordered_out_.empty())
	{
		auto& store = ordered_out_.front();

		if (store.should_send(rtt))
		{
			add_rtt_point(store.creation_time());
			socket_->send_to(store.packet, remote_address_);
			store.reset_activity();
		}
	}

	for (auto& pair : uids_out_)
	{
		auto& store = pair.second;

		if (store.should_send(rtt))
		{
			add_rtt_point(store.creation_time());
			socket_->send_to(store.packet, remote_address_);
			store.reset_activity();
		}
	}

	if (acknew_data_ != nullptr)
	{
		if (acknew_data_->should_send(rtt))
		{
			add_rtt_point(acknew_data_->creation_time());
			socket_->send_to(acknew_data_->packet, remote_address_);
			acknew_data_->reset_activity();
		}
	}
}

bool Connection::pop(sws::Packet* out_packet)
{
	if (inbound_.empty())
	{
		return false;
	}

	auto packet = std::move(inbound_.front());
	inbound_.pop_front();

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

	socket_->send_to(packet, remote_address_);

	disconnect_internal();
}

void Connection::add_rtt_point(const clock::time_point& point)
{
	rtt_points_[rtt_i_++] = clock::now() - point;
	rtt_i_ %= rtt_points_.size();
	rtt_invalid_ = true;
}
