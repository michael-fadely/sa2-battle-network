#pragma once

#include "typedefs.h"

#include "PlayerObject.h" // for PlayerObject
#include "MemoryStruct.h" // for MemStruct

#include "PacketEx.h" // for PacketEx
#include "Networking.h"
#include <array>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "Connection.h"

// TODO: not this
bool round_started();

class PacketBroker
{
	using MessageReader = std::function<bool(nethax::MessageID message_id, pnum_t player_number, sws::Packet& packet)>;
	using MessageWriter = std::function<void(nethax::MessageID message_id, pnum_t player_number, sws::Packet& packet)>;

public:
	explicit PacketBroker(uint timeout);

	void initialize();

	void add_client(std::shared_ptr<Connection> connection);
	void add_server(std::shared_ptr<Connection> connection);

	void receive_loop();

	/**
	 * \brief Requests the specified message type to be added to the outbound packets.
	 * \param message_id The message type.
	 * \param protocol The protocol.
	 * \param allow_dupes If \c true, ignores duplicate types.
	 * \return \c true if added to the outbound packets, \c false on failure (e.g already in outbound packets).
	 */
	bool request(nethax::MessageID message_id, nethax::PacketChannel protocol, bool allow_dupes = false);

	/**
	 * \brief Requests the specified message type to be added to the specified packet.
	 * \param message_id The message type.
	 * \param packet The packet to add the data to.
	 * \param allow_dupes If \c true, ignores duplicate types.
	 * \return \c true on success.
	 */
	bool request(nethax::MessageID message_id, PacketEx& packet, bool allow_dupes = false);

	/**
	 * \brief Appends data to the outbound packets for this frame.
	 * \param message_id The message type.
	 * \param protocol The protocol.
	 * \param packet The packet containing data to append to the mega packet.
	 *  If \c nullptr, the message id will be added with no additional data.
	 * \param allow_dupes If \c true, ignores duplicate types.
	 * \return \c true on success.
	 */
	bool append(nethax::MessageID message_id, nethax::PacketChannel protocol, sws::Packet const* packet, bool allow_dupes = false);

	/**
	 * \brief Finalizes this frame, sending queued packets.
	 */
	void finalize();

	/**
	 * \brief Sends the data stored in \c packet
	 * \param packet The data to be sent.
	 */
	void send(sws::Packet& packet, bool block = false);

	void send_system();
	void send_player();
	void send_menu();

	[[nodiscard]] bool connection_timed_out() const;
	bool wait_for_players(nethax::MessageID message_id);
	void send_ready(nethax::MessageID message_id);
	bool send_ready_and_wait(nethax::MessageID message_id);
	void add_ready(nethax::MessageID message_id, sws::Packet& packet);

	void register_reader(nethax::MessageID message_id, const MessageReader& reader);
	void register_writer(nethax::MessageID message_id, const MessageWriter& writer);

	void set_player_number(pnum_t number);
	[[nodiscard]] auto get_player_number() const { return player_num; }
	[[nodiscard]] bool is_connected() const;

	// FIXME: networking holdover
	[[nodiscard]] bool is_server() const;
	// FIXME: networking holdover
	[[nodiscard]] bool is_bound() const;
	// FIXME: networking holdover
	[[nodiscard]] size_t connection_count() const;

	void disconnect();

	const std::chrono::system_clock::duration connection_timeout;

	[[nodiscard]] std::shared_ptr<ConnectionManager> connection_manager() const;

private:
	struct WaitRequest
	{
		pnum_t count = 0;
	};

	std::shared_ptr<ConnectionManager> connection_manager_;
	std::unordered_map<Connection*, node_t> connection_nodes_;
	std::map<node_t, std::shared_ptr<Connection>> node_connections_;

	std::unordered_map<node_t, std::chrono::system_clock::time_point> keep_alive;
	std::unordered_map<nethax::MessageID, WaitRequest> wait_requests;

	std::unordered_map<nethax::MessageID, MessageReader> message_readers;
	std::unordered_map<nethax::MessageID, MessageWriter> message_writers;

	void reset_packet(PacketEx& packet) const;

	[[nodiscard]] node_t get_free_node() const;

	bool request(nethax::MessageID message_id, PacketEx& packet, const PacketEx& exclude, bool allow_dupes = false);

	// Called by RequestPacket
	// Adds the packet template for message_id to packet
	bool add_to_packet(nethax::MessageID message_id, PacketEx& packet);

	void disconnect(node_t node);
	void read(sws::Packet& packet, node_t node);

	// Read and send System variables
	void send_system(PacketEx& tcp, PacketEx& udp);
	// Read and send Player variables
	void send_player(PacketEx& tcp, PacketEx& udp);
	// Read and send Menu variables
	void send_menu(PacketEx& packet);

	bool receive_system(nethax::MessageID message_id, pnum_t pnum, sws::Packet& packet);
	bool receive_player(nethax::MessageID message_id, pnum_t pnum, sws::Packet& packet);
	bool receive_menu(nethax::MessageID message_id, pnum_t pnum, sws::Packet& packet);

	bool run_message_reader(nethax::MessageID message_id, pnum_t pnum, sws::Packet& packet);

	bool is_server_ = false;

	PacketEx tcp_packet;
	PacketEx udp_packet;

	// HACK: attempting to reduce sent packets
	size_t tcp_packet_size = 0;
	// HACK: attempting to reduce sent packets
	size_t udp_packet_size = 0;

	std::array<PlayerObject, 2> net_player {};

	MemStruct local {};

	// Toggles and things
	bool first_menu_entry = false;

	bool write_player = false;

	bool timed_out = false;
	std::chrono::system_clock::time_point sent_keep_alive;
	pnum_t player_num = 0;
};
