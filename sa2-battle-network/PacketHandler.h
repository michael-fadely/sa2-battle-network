#pragma once

// TODO: Single-protocol modes (TCP/UDP)

#include <deque>
#include <memory>

#include <sws/SocketError.h>
#include <sws/Address.h>
#include <sws/TcpSocket.h>
#include <sws/UdpSocket.h>

#include "typedefs.h"
#include "PacketEx.h"

class PacketHandler
{
public:
	static const node_t broadcast_node = -1;

	PacketHandler();
	~PacketHandler();

	struct Connection
	{
		Connection();
		Connection(const Connection& rhs);
		Connection(Connection&& rhs) noexcept;

		Connection& operator=(const Connection& rhs);
		Connection& operator=(Connection&& rhs) noexcept;

		node_t node;
		std::shared_ptr<sws::TcpSocket> tcp_socket;
		sws::Address udp_address;
	};

	sws::SocketState listen(const sws::Address& address, node_t& node, bool block = true);
	sws::SocketState connect(const sws::Address& address, bool block = true);
	void disconnect();
	void disconnect(node_t node);
	bool is_bound() const;
	bool is_connected() const;
	size_t connection_count() const;
	bool is_server() const;
	ushort get_local_port() const;
	const std::deque<Connection>& connections() const;
	void set_remote_port(node_t node, ushort port);

	sws::SocketState send(PacketEx& packet, node_t node = broadcast_node, node_t node_exclude = broadcast_node);
	sws::SocketState send_tcp(sws::Packet& packet, node_t node = broadcast_node, node_t node_exclude = broadcast_node);
	sws::SocketState send_udp(sws::Packet& packet, node_t node = broadcast_node, node_t node_exclude = broadcast_node);
	sws::SocketState receive_tcp(sws::Packet& packet, const Connection& connection, bool block = false) const;
	sws::SocketState receive_udp(sws::Packet& packet, node_t& node, sws::Address& remote_address, bool block = false);
	bool is_connected(const sws::Address& remote_address) const;

private:
	bool is_bound_;
	bool is_server_;
	sws::TcpSocket listener;
	std::deque<Connection> connections_;
	sws::UdpSocket udp_socket;
	Connection what;

	sws::SocketState bind(const sws::Address& address);
	PacketHandler::Connection get_connection(node_t node);
	void init_communication();
	node_t add_connection();
};
