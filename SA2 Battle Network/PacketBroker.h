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
	/// <param name="isSafe">If set to <c>true</c>, the request will be added to the safe packet.</param>
	/// <param name="allowDuplicates">If set to <c>true</c>, ignores duplicate types.</param>
	/// <returns>true if added to the outbound packets, false on failure (e.g already in outbound packets).</returns>
	bool Request(const nethax::MessageID type, bool isSafe, bool allowDuplicates = false)
	{
		return requestPacket(type, (isSafe) ? tcpPacket : udpPacket, (!isSafe) ? tcpPacket : udpPacket, allowDuplicates);
	}

	/// <summary>
	/// Finalizes this frame, sending queued packets.
	/// </summary>
	void Finalize();

	void SendSystem()	{ sendSystem(tcpPacket, udpPacket); }
	void SendPlayer()	{ sendPlayer(tcpPacket, udpPacket); }
	void SendMenu()		{ sendMenu(tcpPacket, udpPacket); }

	bool ConnectionTimedOut() const;
	bool WaitForPlayers(nethax::MessageID id);
	static void SendReady(const nethax::MessageID id);
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
	void addTypeSent(nethax::MessageID id, ushort length, bool isSafe);
	void addBytesReceived(size_t size);
	void addBytesSent(size_t size);

	// Requests that the packet type packetType is added to packetAddTo if it is not present in packetIsIn
	bool requestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, PacketEx& packetIsIn, bool allowDuplicates = false);
	// Requests that the packet type packetType is added to packetAddTo.
	bool requestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, bool allowDuplicates = false);

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	bool addPacket(const nethax::MessageID packetType, PacketEx& packet);

	void receive(sf::Packet& packet, const bool isSafe);

	// Read and send System variables
	void sendSystem(PacketEx& tcp, PacketEx& udp);
	// Read and send Player varaibles
	void sendPlayer(PacketEx& tcp, PacketEx& udp);
	// Read and send Menu variables
	void sendMenu(PacketEx& tcp, PacketEx& udp);

	bool receiveInput(const nethax::MessageID type, sf::Packet& packet);
	bool receiveSystem(const nethax::MessageID type, sf::Packet& packet);
	bool receivePlayer(const nethax::MessageID type, sf::Packet& packet);
	bool receiveMenu(const nethax::MessageID type, sf::Packet& packet);
	
	void preReceive();
	void postReceive() const;
	void writeSpecials() const;
	void writeTimeStop() const;

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
