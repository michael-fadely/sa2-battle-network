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

		const clock::time_point& creation_time() const;

		bool should_send(const clock::duration& duration) const;
		void reset_activity();
	};

private:
	std::shared_ptr<sws::UdpSocket> socket;
	bool connected_ = false;

	// to be used for disconnecting
	ConnectionManager* parent = nullptr;

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
	sws::Address remote_address;

	Connection(std::shared_ptr<sws::UdpSocket> socket_, ConnectionManager* parent_, sws::Address remote_address_);
	Connection(Connection&& other) noexcept;

	Connection& operator=(Connection&& other) noexcept;

	sws::SocketState send(sws::Packet& packet, bool block = false);

	sws::SocketState store_inbound(sws::Packet& packet);
	bool handled(reliable::reliable_t type, sequence_t sequence);
	void prune();
	const clock::duration& round_trip_time();
	void update();
	bool pop(sws::Packet& packet);

	bool connected() const;

private:
	void remove_outbound(reliable::reliable_t type, sequence_t sequence);
	void add_rtt_point(const clock::time_point& point);
};
