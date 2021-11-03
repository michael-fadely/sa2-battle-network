#include "stdafx.h"

#include <thread>
#include <stdexcept>

#include "reliable.h"

#include "ConnectionManager.h"

using namespace sws;
using namespace std::chrono;
using namespace reliable;

using protover_t = uint16_t;

static constexpr protover_t PROTOCOL_VERSION = 1;

ConnectionManager::ConnectionManager()
	: socket_(std::make_shared<UdpSocket>(false))
{
}

SocketState ConnectionManager::host(const Address& address)
{
	if (is_bound_)
	{
		return SocketState::done;
	}

	const SocketState result = socket_->bind(address);
	is_bound_ = result == SocketState::done;
	return result;
}

SocketState ConnectionManager::listen(std::shared_ptr<Connection>& connection)
{
	if (!is_bound_)
	{
		throw std::runtime_error("cannot listen for connections on unbound socket");
	}

	Packet in, out;
	Address address;

	SocketState result = socket_->receive_from(in, address);

	if (result != SocketState::done)
	{
		return result;
	}

	while (!in.end())
	{
		manage_id id = manage_id::eop;
		in >> id;

		if (id == manage_id::eop)
		{
			break;
		}

		if (id != manage_id::connect)
		{
			result = SocketState::in_progress;
			break;
		}

		protover_t version = 0;
		in >> version;

		if (version == PROTOCOL_VERSION)
		{
			result = SocketState::done;
			out << manage_id::connected << manage_id::eop;
		}
		else
		{
			result = SocketState::in_progress;
			out << manage_id::bad_version << PROTOCOL_VERSION << manage_id::eop;
		}

		socket_->send_to(out, address);
	}

	connection = std::make_shared<Connection>(socket_, this, address);
	connections_[address] = connection;
	return result;
}

SocketState ConnectionManager::connect(const Address& host_address, std::shared_ptr<Connection>& connection)
{
	if (is_connected_)
	{
		return SocketState::done;
	}

	SocketState result;

	if (!is_bound_)
	{
		const Address bind_address = Address::get_addresses("localhost", Socket::any_port, host_address.family)[0];
		result = socket_->bind(bind_address);

		is_bound_ = result == SocketState::done;

		if (!is_bound_)
		{
			return result;
		}
	}

	if (clock::now() - last_connect_ >= 500ms)
	{
		last_connect_ = clock::now();

		Packet out;
		out << manage_id::connect << PROTOCOL_VERSION << manage_id::eop;

		result = socket_->send_to(out, host_address);

		if (result != SocketState::done)
		{
			return result;
		}
	}

	Packet in;
	Address recv_address;

	result = socket_->receive_from(in, recv_address);

	if (result != SocketState::done)
	{
		return result;
	}

	if (recv_address != host_address)
	{
		return SocketState::in_progress;
	}

	while (!in.end())
	{
		manage_id id = manage_id::eop;
		in >> id;

		switch (id)
		{
			case manage_id::connected:
				break;

			case manage_id::bad_version:
				throw std::runtime_error("version mismatch with server");

			case manage_id::eop:
				in.clear();
				continue;

			default:
				throw std::runtime_error("invalid message id for connection process");
		}
	}

	connection = std::make_shared<Connection>(socket_, this, host_address);
	connections_[host_address] = connection;
	is_connected_ = true;
	return SocketState::done;
}

SocketState ConnectionManager::receive(bool block, const size_t count)
{
	size_t i = 0;

	Packet packet;
	Address remote_address;

	SocketState result = SocketState::done;

	while ((block && result == SocketState::in_progress) || result == SocketState::done)
	{
		result = socket_->receive_from(packet, remote_address);

		if (result == SocketState::in_progress)
		{
			std::this_thread::sleep_for(1ms);
			continue;
		}

		const auto it = connections_.find(remote_address);

		if (it == connections_.end())
		{
			packet.clear();
			result = SocketState::in_progress;
		}
		else
		{
			result = it->second->store_inbound(packet);
		}

		// TODO: decouple from pure ACKs
		if (count && ++i >= count)
		{
			break;
		}
	}

	return result;
}

bool ConnectionManager::is_bound() const
{
	return is_bound_;
}

bool ConnectionManager::connected() const
{
	return !connections_.empty();
}
