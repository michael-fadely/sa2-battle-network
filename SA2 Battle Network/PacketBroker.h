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
		return RequestPacket(type, (isSafe) ? safe : fast, (!isSafe) ? safe : fast, allowDuplicates);
	}

	/// <summary>
	/// Finalizes this frame, sending queued packets.
	/// </summary>
	void Finalize();

	void SendSystem()	{ SendSystem(safe, fast); }
	void SendPlayer()	{ SendPlayer(safe, fast); }
	void SendMenu()		{ SendMenu(safe, fast); }

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

	// Requests that the packet type packetType is added to packetAddTo if it is not present in packetIsIn
	bool RequestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, PacketEx& packetIsIn, bool allowDuplicates = false);
	// Requests that the packet type packetType is added to packetAddTo.
	bool RequestPacket(const nethax::MessageID packetType, PacketEx& packetAddTo, bool allowDuplicates = false);

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	bool AddPacket(const nethax::MessageID packetType, PacketEx& packet);

	void Receive(sf::Packet& packet, const bool safe);

	// Read and send System variables
	void SendSystem(PacketEx& safe, PacketEx& fast);
	// Read and send Player varaibles
	void SendPlayer(PacketEx& safe, PacketEx& fast);
	// Read and send Menu variables
	void SendMenu(PacketEx& safe, PacketEx& fast);

	bool ReceiveInput(const nethax::MessageID type, sf::Packet& packet);
	bool ReceiveSystem(const nethax::MessageID type, sf::Packet& packet);
	bool ReceivePlayer(const nethax::MessageID type, sf::Packet& packet);
	bool ReceiveMenu(const nethax::MessageID type, sf::Packet& packet);
	
	void PreReceive();
	void PostReceive() const;
	void writeSpecials() const;
	void writeTimeStop() const;

	PacketEx safe, fast;
	PlayerObject recvPlayer, sendPlayer;

	// Used for comparison to determine what to send.
	MemStruct local;

	// Toggles and things
	bool firstMenuEntry;
	bool wroteP2Start;

	// Set in ReceivePlayer to true upon receival of a valid player message.
	bool writePlayer;

	// Set in SendSystem on level change to true if playing a relevant character.
	// (Sonic, Shadow, Amy, Metalsonic)
	bool sendSpinTimer;

	bool timedOut;
	uint sentKeepalive, receivedKeepalive;
	ushort lastSequence;
};
