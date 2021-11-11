#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <sws/UdpSocket.h>
#include "Connection.h"

class ConnectionManager
{
	using clock = std::chrono::high_resolution_clock;

	std::shared_ptr<sws::UdpSocket> socket_;

	std::unordered_map<sws::Address, std::shared_ptr<Connection>> connections_;

	bool is_bound_ = false;

	clock::time_point last_connect_;

	sws::Packet inbound_packet_;

public:
	explicit ConnectionManager();

	sws::SocketState host(const sws::Address& address);
	sws::SocketState listen(std::shared_ptr<Connection>* out_connection);
	sws::SocketState connect(const sws::Address& host_address, std::shared_ptr<Connection>* out_connection);
	void disconnect();
	void disconnect(const std::shared_ptr<Connection>& connection);
	sws::SocketState receive(bool block = false, size_t count = 0);

	[[nodiscard]] bool is_bound() const;
	[[nodiscard]] bool is_connected() const;
	[[nodiscard]] size_t connection_count() const;
};
