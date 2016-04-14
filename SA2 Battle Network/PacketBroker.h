#pragma once

#include "typedefs.h"

#include "PlayerObject.h"			// for PlayerObject
#include "MemoryStruct.h"			// for MemStruct

#include <SFML/Network.hpp>			// for sf::Packet
#include "PacketExtensions.h"		// for PacketEx
#include "Networking.h"
#include <unordered_map>

class PacketBroker
{
public:
	explicit PacketBroker(uint timeout);

	void Initialize();
	void ReceiveLoop();
	/// <summary>
	/// Requests the specified message type to be added to the outbound packets.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="isSafe">If <c>true</c>, the request will be added to the safe packet.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> if added to the outbound packets, <c>false</c> on failure (e.g already in outbound packets).</returns>
	bool Request(nethax::MessageID type, nethax::Protocol protocol, bool allowDupes = false);	
	/// <summary>
	/// Requests the specified message type to be added to the specified packet.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="packet">The packet to add the data to.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> on success.</returns>
	bool Request(nethax::MessageID type, PacketEx& packet, bool allowDupes = false);
	/// <summary>
	/// Adds the specific message type to the specified packet and sends immediately on success.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="packet">The packet to add the data to.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> on success.</returns>
	bool Demand(nethax::MessageID type, PacketEx& packet, bool allowDupes = false);
	bool Demand(nethax::MessageID type, nethax::Protocol protocol, bool allowDupes = false);

	/// <summary>
	/// Finalizes this frame, sending queued packets.
	/// </summary>
	void Finalize();	
	/// <summary>
	/// Sends the data stored in <c>packet</c>
	/// </summary>
	/// <param name="packet">The data to be sent.</param>
	void Send(PacketEx& packet);

	void SendSystem()	{ sendSystem(tcpPacket, udpPacket); }
	void SendPlayer()	{ sendPlayer(tcpPacket, udpPacket); }
	void SendMenu()		{ sendMenu(tcpPacket, udpPacket); }

	bool ConnectionTimedOut() const;
	bool WaitForPlayers(nethax::MessageID id);
	static void SendReady(nethax::MessageID id);
	static void AddReady(nethax::MessageID id, sf::Packet& packet);
	void SetConnectTime();
	void ToggleNetStat(bool toggle);
	void SaveNetStat() const;

	const uint ConnectionTimeout;
	ControllerData recvInput, sendInput;
	PolarCoord recvAnalog, sendAnalog;

private:
	// TODO: Consider an integer instead of a boolean for multiple wait requests.
	std::unordered_map<nethax::MessageID, bool> WaitRequests;

	bool netStat;
	std::map<nethax::MessageID, nethax::MessageStat> sendStats;
	std::map<nethax::MessageID, nethax::MessageStat> recvStats;
	size_t received_packets, received_bytes;
	size_t sent_packets, sent_bytes;

	static void addType(nethax::MessageStat& stat, nethax::MessageID id, ushort size, bool isSafe);
	void addTypeReceived(nethax::MessageID id, ushort length, bool isSafe);
	void addTypeSent(nethax::MessageID id, ushort length, nethax::Protocol protocol);
	void addBytesReceived(size_t size);
	void addBytesSent(size_t size);

	bool request(nethax::MessageID type, PacketEx& packet, PacketEx& exclude, bool allowDupes = false);
	bool request(nethax::MessageID type, PacketEx& packet, bool allowDupes = false);

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	bool addPacket(nethax::MessageID packetType, PacketEx& packet);

	void receive(sf::Packet& packet, nethax::Protocol protocol);

	// Read and send System variables
	void sendSystem(PacketEx& tcp, PacketEx& udp);
	// Read and send Player varaibles
	void sendPlayer(PacketEx& tcp, PacketEx& udp);
	// Read and send Menu variables
	void sendMenu(PacketEx& tcp, PacketEx& udp);

	bool receiveInput(nethax::MessageID type, sf::Packet& packet);
	bool receiveSystem(nethax::MessageID type, sf::Packet& packet);
	bool receivePlayer(nethax::MessageID type, sf::Packet& packet);
	bool receiveMenu(nethax::MessageID type, sf::Packet& packet);

	PacketEx tcpPacket, udpPacket;
	PlayerObject inPlayer, outPlayer;

	// Used for comparison to determine what to send.
	MemStruct local;

	// Toggles and things
	bool firstMenuEntry;
	bool wroteP2Start;

	// Set in ReceivePlayer to true upon receival of a valid player message.
	bool writePlayer;

	bool timedOut;
	uint sentKeepalive, receivedKeepalive;
	ushort lastSequence;
};
