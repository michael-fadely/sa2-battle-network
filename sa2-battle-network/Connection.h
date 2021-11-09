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
		clock::time_point last_active;

	public:
		sequence_t sequence;
		sws::Packet packet;

		Store() = default;
		Store(sequence_t sequence, sws::Packet packet);
		Store(Store&& other) noexcept;
		Store& operator=(Store&& other) noexcept;

		[[nodiscard]] clock::time_point creation_time() const;

		[[nodiscard]] bool should_send(const clock::duration& duration) const;
		void reset_activity();
	};

private:
	// to be used for disconnecting
	ConnectionManager* parent = nullptr;

	std::shared_ptr<sws::UdpSocket> socket;
	sws::Address remote_address_;

	bool is_connected_ = false;

	std::deque<sws::Packet> inbound;

	std::deque<Store> ordered_out;
	std::unordered_map<sequence_t, Store> uids_out;

	std::unordered_map<sequence_t, clock::time_point> seqs_in;
	std::unordered_map<sequence_t, clock::time_point> uids_in;

	sequence_t seq_in;
	sequence_t acknew_in;
	sequence_t faf_in;

	sequence_t seq_out;
	sequence_t uid_out;
	sequence_t faf_out;

	sequence_t acknew_out;
	std::unique_ptr<Store> acknew_data;

	bool rtt_invalid = false;
	size_t rtt_i = 0;
	std::array<clock::duration, 50> rtt_points {};
	clock::duration current_rtt {};

public:
	Connection(ConnectionManager* parent_, std::shared_ptr<sws::UdpSocket> socket_, sws::Address remote_address_);
	Connection(Connection&& other) noexcept;

	Connection& operator=(Connection&& other) noexcept;

	sws::SocketState send(sws::Packet& packet, bool block = false);

	sws::SocketState store_inbound(sws::Packet& packet);

private:
	bool handled(reliable::reliable_t type, sequence_t sequence);
	void prune();

public:
	clock::duration round_trip_time();
	void update();
	bool pop(sws::Packet* out_packet);

	[[nodiscard]] bool is_connected() const;

	[[nodiscard]] const sws::Address& remote_address() const;
	void disconnect();

private:
	void disconnect_internal();
	void remove_outbound(reliable::reliable_t type, sequence_t sequence);
	void add_rtt_point(const clock::time_point& point);
};
