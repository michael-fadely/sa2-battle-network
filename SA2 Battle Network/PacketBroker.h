#pragma once

#include "typedefs.h"

#include "PlayerObject.h"			// for PlayerObject
#include "MemoryStruct.h"			// for MemStruct

#include <SFML/Network.hpp>			// for sf::Packet
#include "PacketEx.h"		// for PacketEx
#include "Networking.h"
#include <unordered_map>
#include <functional>

// TODO: not this
bool roundStarted();

class PacketBroker
{
	typedef std::function<bool(nethax::MessageID, int, sf::Packet&)> MessageHandler;

public:
	explicit PacketBroker(uint timeout);

	void Initialize();
	void ReceiveLoop();
	/// <summary>
	/// Requests the specified message type to be added to the outbound packets.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="protocol">The protocol.</param>
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
	/// Appends data to the outbound packets for this frame.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="protocol">The protocol.</param>
	/// <param name="packet">The packet containing data to append to the mega packet.
	/// If <c>nullptr</c>, the message id will be added with no additional data.</param>
	/// <param name="allowDupes">If <c>true</c>, ignores duplicate types.</param>
	/// <returns><c>true</c> on success.</returns>
	bool Append(nethax::MessageID type, nethax::Protocol protocol, sf::Packet const* packet, bool allowDupes = false);

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
	void SendReady(nethax::MessageID id);
	void AddReady(nethax::MessageID id, sf::Packet& packet);
	void SetConnectTime();
	void ToggleNetStat(bool toggle);
	void SaveNetStat() const;
	void AddTypeReceived(nethax::MessageID id, size_t length, bool isSafe);
	void AddTypeSent(nethax::MessageID id, size_t length, nethax::Protocol protocol);

	void RegisterMessageHandler(nethax::MessageID type, MessageHandler func);

	const uint ConnectionTimeout;
	ControllerData recvInput, sendInput;
	PolarCoord recvAnalog, sendAnalog;

private:
	// TODO: Consider an integer instead of a boolean for multiple wait requests.
	std::unordered_map<nethax::MessageID, bool> waitRequests;
	std::unordered_map<nethax::MessageID, MessageHandler> messageHandlers;

	bool netStat;
	std::map<nethax::MessageID, nethax::MessageStat> sendStats;
	std::map<nethax::MessageID, nethax::MessageStat> recvStats;
	size_t received_packets, received_bytes;
	size_t sent_packets, sent_bytes;

	void addBytesReceived(size_t size);
	void addBytesSent(size_t size);

	static void addType(nethax::MessageStat& stat, nethax::MessageID id, ushort size, bool isSafe);
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

	bool runMessageHandler(nethax::MessageID type, int pnum, sf::Packet& packet);

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
