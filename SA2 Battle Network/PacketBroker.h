#pragma once

#include <LazyTypedefs.h>

#include "PlayerObject.h"			// for PlayerObject
#include "MemoryStruct.h"			// for MemStruct

#include <SFML/Network.hpp>			// for sf::Packet
#include "PacketExtensions.h"		// for PacketEx

class PacketBroker
{
public:
	PacketBroker(uint timeout);
	void Initialize();

	//
	// Methods
	//

	void RecvLoop();

	/// <summary>
	/// Requests the specified message type to be added to the outbound packets.
	/// </summary>
	/// <param name="type">The message type.</param>
	/// <param name="isSafe">If set to <c>true</c>, the request will be added to the safe packet.</param>
	/// <returns>true if added to the outbound packets, false on failure (e.g already in outbound packets).</returns>
	bool inline PacketBroker::Request(uint8 type, bool isSafe)
	{
		if (isSafe)
			return RequestPacket(type, safe, fast);
		else
			return RequestPacket(type, fast, safe);
	}

	/// <summary>
	/// Finalizes this instance, sending queued packets.
	/// </summary>
	void Finalize();

	inline void SendSystem()	{ SendSystem(safe, fast); }
	inline void SendPlayer()	{ SendPlayer(safe, fast); }
	inline void SendMenu()		{ SendMenu(safe, fast); }

	// HACK: Dirty, dirty hack.
	bool isClientReady;
	const uint Timeout;
	ControllerData recvInput, sendInput;

private:
	//
	// Methods
	//

	// Requests that the packet type packetType is added to packetAddTo if it is not present in packetIsIn
	bool RequestPacket(const uint8 packetType, PacketEx& packetAddTo, PacketEx& packetIsIn);
	// Requests that the packet type packetType is added to packetAddTo.
	bool RequestPacket(const uint8 packetType, PacketEx& packetAddTo);

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	bool AddPacket(const uint8 packetType, PacketEx& packet);

	void Receive(sf::Packet& packet, const bool safe);

	// Read and send System variables
	void SendSystem(PacketEx& safe, PacketEx& fast);
	// Read and send Player varaibles
	void SendPlayer(PacketEx& safe, PacketEx& fast);
	// Read and send Menu variables
	void SendMenu(PacketEx& safe, PacketEx& fast);

	// Receive and write Input
	bool ReceiveInput(uint8 type, sf::Packet& packet);
	// Receive game/system variables
	bool ReceiveSystem(uint8 type, sf::Packet& packet);
	// Receive and queue write of Player variables
	bool ReceivePlayer(uint8 type, sf::Packet& packet);
	// Receive and write Menu variables
	bool ReceiveMenu(uint8 type, sf::Packet& packet);

	void PreReceive();
	void PostReceive();

	// Pretty much all of these are just so I can be lazy
	void writeRings();
	void writeSpecials();
	void writeTimeStop();
	void ToggleSplitscreen();
	// Returns true if the player has been teleported.
	bool Teleport();

	//
	// Members
	//

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
};
