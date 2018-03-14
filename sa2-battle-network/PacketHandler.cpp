#include "stdafx.h"

#include <algorithm>
#include "PacketHandler.h"

// TODO: Handle disconnects
// TODO: Handle Connections that have invalid nodes on send

PacketHandler::PacketHandler() : bound(false), host(false), what({})
{
	listener.blocking(false);
	udp_socket.blocking(false);
}
PacketHandler::~PacketHandler()
{
	disconnect();
}

sws::SocketState PacketHandler::listen(const sws::Address& address, node_t& node, bool block)
{
	sws::SocketState result;

	if (!bound)
	{
		bind(address);

		do
		{
			result = listener.listen();
		} while (block && result == sws::SocketState::in_progress);

		if (result == sws::SocketState::error)
		{
			throw sws::SocketException("listen failed", listener.native_error());
		}

		if (!block && result != sws::SocketState::done)
		{
			return result;
		}

		bound = true;
	}

	init_communication();

	do
	{
		result = listener.accept(*what.tcp_socket);
	} while (block && result == sws::SocketState::in_progress);

	if (result == sws::SocketState::error)
	{
		throw sws::SocketException("accept failed", listener.native_error());
	}

	if (result != sws::SocketState::done)
	{
		return result;
	}

	host = true;

	// Pull the remote address out of the socket
	// and store it for later use with UDP.
	what.udp_address = what.tcp_socket->remote_address();
	node = add_connection();

	return result;
}
sws::SocketState PacketHandler::connect(const sws::Address& address, bool block)
{
	sws::SocketState result = sws::SocketState::done;

	if (is_connected())
	{
		return result;
	}

	if (!bound)
	{
		const sws::Address local_address
			= sws::Address::get_addresses("localhost", sws::Socket::any_port, sws::AddressFamily::inet)[0];

		bind(local_address);
		bound = true;
	}

	what.udp_address = address;

	init_communication();

	do
	{
		result = what.tcp_socket->connect(what.udp_address);
	} while (block && result == sws::SocketState::in_progress);

	if (result == sws::SocketState::error)
	{
		throw sws::SocketException("connect failed", what.tcp_socket->native_error());
	}

	if (!block && result != sws::SocketState::done)
	{
		return result;
	}

	host = false;
	add_connection();
	return result;
}
void PacketHandler::disconnect()
{
	for (auto& i : connections_)
	{
		i.tcp_socket->close();
		delete i.tcp_socket;
	}

	connections_.clear();
	udp_socket.close();
	listener.close();

	bound = false;
}

void PacketHandler::disconnect(node_t node)
{
	if (connections_.size() == 1)
	{
		disconnect();
	}

	auto c = find_if(connections_.begin(), connections_.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (c == connections_.end())
	{
		return;
	}

	c->tcp_socket->close();
	connections_.erase(c);
}

bool PacketHandler::is_bound() const
{
	return bound;
}

bool PacketHandler::is_connected() const
{
	return !connections_.empty();
}

size_t PacketHandler::connection_count() const
{
	return connections_.size();
}

bool PacketHandler::is_server() const
{
	return host;
}

sws::SocketState PacketHandler::bind(const sws::Address& address)
{
	sws::SocketState result;

	if ((result = udp_socket.bind(address)) != sws::SocketState::done)
	{
		if (result == sws::SocketState::error)
		{
			throw sws::SocketException("Failed to bind UDP socket", sws::Socket::get_native_error());
		}
	}

	if ((result = listener.bind(address)) != sws::SocketState::done)
	{
		if (result == sws::SocketState::error)
		{
			throw sws::SocketException("Failed to bind TCP socket", sws::Socket::get_native_error());
		}
	}

	return result;
}

PacketHandler::Connection PacketHandler::get_connection(node_t node)
{
	const auto c = find_if(connections_.begin(), connections_.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (c == connections_.end())
	{
		return {};
	}

	return *c;
}

void PacketHandler::init_communication()
{
	if (what.tcp_socket != nullptr)
	{
		return;
	}

	auto socket = new sws::TcpSocket();
	what.tcp_socket = socket;
	socket->blocking(false);
}

node_t PacketHandler::add_connection()
{
	auto node = host ? static_cast<node_t>(connections_.size()) + 1 : 0;
	what.node = node;
	connections_.push_back(what);
	what = {};
	return node;
}

ushort PacketHandler::get_local_port() const
{
	return udp_socket.local_address().port;
}

const std::deque<PacketHandler::Connection>& PacketHandler::connections() const
{
	return connections_;
}

void PacketHandler::set_remote_port(node_t node, ushort port)
{
	auto connection = find_if(connections_.begin(), connections_.end(), [node](Connection& c)
	{
		return c.node == node;
	});

	if (connection == connections_.end())
	{
		throw std::exception("No connections exist with the specified node.");
	}

	connection->udp_address.port = port;
}

sws::SocketState PacketHandler::send(PacketEx& packet, node_t node, node_t node_exclude)
{
	if (!packet.is_empty())
	{
		return packet.protocol == nethax::Protocol::tcp ? send_tcp(packet, node, node_exclude) : send_udp(packet, node, node_exclude);
	}

	return sws::SocketState::in_progress;
}
sws::SocketState PacketHandler::send_tcp(sws::Packet& packet, node_t node, node_t node_exclude)
{
	sws::SocketState result = sws::SocketState::in_progress;

	if (node < 0)
	{
		for (auto& i : connections_)
		{
			if (i.node == node_exclude)
			{
				continue;
			}

			result = i.tcp_socket->send(packet);

			if (result == sws::SocketState::error)
			{
				throw std::exception("Data send failure.");
			}
		}

		return result;
	}

	auto connection = get_connection(node);

	if (connection.tcp_socket == nullptr)
	{
		throw std::exception("No connections exist with the specified node.");
	}

	result = connection.tcp_socket->send(packet);

	if (result == sws::SocketState::error)
	{
		throw std::exception("Data send failure.");
	}

	return result;
}
sws::SocketState PacketHandler::send_udp(sws::Packet& packet, node_t node, node_t node_exclude)
{
	sws::SocketState result = sws::SocketState::in_progress;

	if (node < 0)
	{
		for (auto& i : connections_)
		{
			if (i.node == node_exclude)
			{
				continue;
			}

			result = udp_socket.send_to(packet, i.udp_address);

			if (result != sws::SocketState::done)
			{
				throw std::exception("Data send failure.");
			}
		}

		return result;
	}

	const auto connection = get_connection(node);

	if (connection.tcp_socket == nullptr)
	{
		throw std::exception("No connections exist with the specified node.");
	}

	result = udp_socket.send_to(packet, connection.udp_address);

	if (result != sws::SocketState::done)
	{
		throw std::exception("Data send failure.");
	}

	return result;

}

sws::SocketState PacketHandler::receive_tcp(sws::Packet& packet, const Connection& connection, bool block) const
{
	sws::SocketState result = sws::SocketState::in_progress;

	if (is_connected())
	{
		do
		{
			result = connection.tcp_socket->receive(packet);
		} while (block && result == sws::SocketState::in_progress);

		if (result == sws::SocketState::error)
		{
			throw sws::SocketException("receive failed", connection.tcp_socket->native_error());
		}
	}

	return result;
}
sws::SocketState PacketHandler::receive_udp(sws::Packet& packet, node_t& node, sws::Address& remote_address, bool block)
{
	sws::SocketState result = sws::SocketState::in_progress;

	if (is_connected())
	{
		do
		{
			result = udp_socket.receive_from(packet, remote_address);
		} while (block && result == sws::SocketState::in_progress);

		if (result == sws::SocketState::error)
		{
			throw sws::SocketException("receive_from failed", udp_socket.native_error());
		}

		const auto connection = std::find_if(connections_.begin(), connections_.end(), [remote_address](auto c)
		{
			return c.udp_address.port == remote_address.port && c.udp_address.address == remote_address.address;
		});

		node = connection == connections_.end() ? broadcast_node : connection->node;
	}

	return result;
}

bool PacketHandler::is_connected(const sws::Address& remote_address) const
{
	return std::any_of(connections_.begin(), connections_.end(), [remote_address](auto c)
	{
		return c.udp_address.port == remote_address.port && c.udp_address.address == remote_address.address;
	});
}
