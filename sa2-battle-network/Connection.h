#pragma once

#include <chrono>
#include <deque>
#include <memory>
#include <unordered_map>

#include <sws/Address.h>
#include <sws/UdpSocket.h>

#include "sequence_t.h"
#include "reliable.h"

class ConnectionManager;

class Connection
{
public:
	using clock = std::chrono::high_resolution_clock;

	struct Store
	{
	private:
		clock::time_point creation_time_;
		clock::time_point last_active_;

	public:
		sequence_t sequence;
		sws::Packet packet;

		Store() = default;
		Store(sequence_t sequence, sws::Packet packet);
		Store(Store&& other) noexcept = default;

		~Store() = default;

		Store& operator=(Store&& other) noexcept = default;

		Store(const Store&) = delete;
		Store& operator=(const Store&) = delete;

		[[nodiscard]] clock::time_point creation_time() const;

		[[nodiscard]] bool should_send(const clock::duration& duration) const;
		void reset_activity();
	};

private:
	// to be used for disconnecting
	ConnectionManager* parent_ = nullptr;

	std::shared_ptr<sws::UdpSocket> socket_;
	sws::Address remote_address_;

	bool is_connected_ = false;

	std::deque<sws::Packet> inbound_;

	std::deque<Store> ordered_out_;
	std::unordered_map<sequence_t, clock::time_point> ordered_in_;

	std::unordered_map<sequence_t, Store> uids_out_;
	std::unordered_map<sequence_t, clock::time_point> uids_in_;

	sequence_t seq_in_;
	sequence_t ack_newest_in_;
	sequence_t faf_in_;

	sequence_t seq_out_;
	sequence_t uid_out_;
	sequence_t faf_out_;

	sequence_t ack_newest_out_;
	std::unique_ptr<Store> ack_newest_data_;

	bool rtt_invalid_ = false;
	size_t rtt_i_ = 0;
	std::array<clock::duration, 60> rtt_points_ {};
	clock::duration current_rtt_ {};

public:
	Connection(ConnectionManager* parent, std::shared_ptr<sws::UdpSocket> socket, sws::Address remote_address);
	Connection(Connection&& other) noexcept;

	Connection& operator=(Connection&& other) noexcept;

	sws::SocketState send(sws::Packet& packet, bool block = false);

	sws::SocketState store_inbound(sws::Packet& packet);

private:
	bool handled(reliable::reliable_t type, sequence_t sequence);
	void prune_inbound_ids();

public:
	clock::duration round_trip_time();
	void update_outbound();
	bool pop(sws::Packet* out_packet);

	[[nodiscard]] bool is_connected() const;

	[[nodiscard]] const sws::Address& remote_address() const;
	void disconnect();

private:
	void disconnect_internal();
	void remove_outbound(reliable::reliable_t reliable_type, sequence_t sequence);
	void add_rtt_point(const clock::time_point& point);
};
