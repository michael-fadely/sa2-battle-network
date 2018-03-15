#pragma once

#include "typedefs.h"

#include "PlayerObject.h" // for PlayerObject
#include "MemoryStruct.h" // for MemStruct

#include "PacketEx.h" // for PacketEx
#include "Networking.h"
#include <unordered_map>
#include <functional>
#include <chrono>

// TODO: not this
bool round_started();

class PacketBroker
{
	using MessageHandler = std::function<bool(nethax::MessageID, pnum_t, sws::Packet&)>;

public:
	explicit PacketBroker(uint timeout);

	void initialize();
	void receive_loop();

	/// <summary>
	/// Requests the specified message type to be added to the outbound packets.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="protocol">The protocol.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> if added to the outbound packets, <c>false</c> on failure (e.g already in outbound packets).</returns>
	bool request(nethax::MessageID type, nethax::Protocol protocol, bool allow_dupes = false);

	/// <summary>
	/// Requests the specified message type to be added to the specified packet.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="packet">The packet to add the data to.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> on success.</returns>
	bool request(nethax::MessageID type, PacketEx& packet, bool allow_dupes = false);

	/// <summary>
	/// Appends data to the outbound packets for this frame.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="protocol">The protocol.</param>
	/// <param name="packet">The packet containing data to append to the mega packet.
	/// If <c>nullptr</c>, the message id will be added with no additional data.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> on success.</returns>
	bool append(nethax::MessageID type, nethax::Protocol protocol, sws::Packet const* packet, bool allow_dupes = false);

	/// <summary>
	/// Finalizes this frame, sending queued packets.
	/// </summary>
	void finalize();

	/// <summary>
	/// Sends the data stored in <c>packet</c>
	/// </summary>
	/// <param name="packet">The data to be sent.</param>
	void send(PacketEx& packet);

	void send_system();
	void send_player();
	void send_menu();

	bool connection_timed_out() const;
	bool wait_for_players(nethax::MessageID id);
	void send_ready(nethax::MessageID id);
	bool send_ready_and_wait(nethax::MessageID id);
	void add_ready(nethax::MessageID id, sws::Packet& packet);
	void set_connect_time();
	void toggle_netstat(bool value);
	void save_netstat() const;
	void add_type_received(nethax::MessageID id, size_t size, bool is_safe);
	void add_type_sent(nethax::MessageID id, size_t size, nethax::Protocol protocol);

	void register_message_handler(nethax::MessageID type, const MessageHandler& func);

	void set_player_number(pnum_t number);
	auto get_player_number() const { return player_num; }

	const std::chrono::system_clock::duration connection_timeout;

private:
	struct WaitRequest
	{
		pnum_t count = 0;
	};

	std::unordered_map<node_t, std::chrono::system_clock::time_point> keep_alive;
	std::unordered_map<node_t, ushort> sequences;
	std::unordered_map<nethax::MessageID, WaitRequest> wait_requests;
	std::unordered_map<nethax::MessageID, MessageHandler> message_handlers;

	bool netstat;
	std::map<nethax::MessageID, nethax::MessageStat> send_stats;
	std::map<nethax::MessageID, nethax::MessageStat> recv_stats;

	size_t received_packets = 0;
	size_t received_bytes   = 0;
	size_t sent_packets     = 0;
	size_t sent_bytes       = 0;

	void add_bytes_received(size_t size);
	void add_bytes_sent(size_t size);

	static void add_type(nethax::MessageStat& stat, ushort size, bool is_safe);
	bool request(nethax::MessageID type, PacketEx& packet, PacketEx& exclude, bool allow_dupes = false);

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	bool add_packet(nethax::MessageID packet_type, PacketEx& packet);

	void receive(sws::Packet& packet, node_t node, nethax::Protocol protocol);

	// Read and send System variables
	void send_system(PacketEx& tcp, PacketEx& udp);
	// Read and send Player varaibles
	void send_player(PacketEx& tcp, PacketEx& udp);
	// Read and send Menu variables
	void send_menu(PacketEx& packet);

	bool receive_system(nethax::MessageID type, pnum_t pnum, sws::Packet& packet);
	bool receive_player(nethax::MessageID type, pnum_t pnum, sws::Packet& packet);
	bool receive_menu(nethax::MessageID   type, pnum_t pnum, sws::Packet& packet);

	bool run_message_handler(nethax::MessageID type, pnum_t pnum, sws::Packet& packet);

	PacketEx tcp_packet;
	PacketEx udp_packet;
	PlayerObject net_player[2];

	// Used for comparison to determine what to send.
	MemStruct local {};

	// Toggles and things
	bool first_menu_entry = false;

	// Set in ReceivePlayer to true upon receival of a valid player message.
	bool write_player = false;

	bool timed_out = false;
	std::chrono::system_clock::time_point sent_keep_alive;
	pnum_t player_num = 0;
};
