#pragma once

// Defines
#ifndef RECEIVED
#define RECEIVED RECV_CONCISE
#endif

#define RECV_VERBOSE(type) case type: cout << ">> [" << millisecs() << "] " #type << endl
#define RECV_CONCISE(type) case type:


#include "MemoryManagement.h"
#include "ModLoaderExtensions.h"
#include "AddressList.h"
#include "PlayerObject.h"
#include "MemoryStruct.h"

#include <SFML\Network.hpp>
#include "PacketExtensions.h"

class MemoryHandler
{
public:
	MemoryHandler();

	//
	// Methods
	//

	void Initialize();

	void RecvLoop();
	void SendLoop();

	// Reads the frame count from memory into thisFrame
	inline void GetFrame() { thisFrame = FrameCount; }
	// Sets lastFrame to thisFrame
	inline void SetFrame() { lastFrame = thisFrame; }
	// Designed exclusively to be used externally. Returns the current menu.
	// This function does frame synchronization to ensure you don't catch the value
	// mid-operation. Used by class Program
	const unsigned int GetCurrentMenu();

	// Requests that the packet type packetType is added to packetAddTo if it is not present in packetIsIn
	const bool RequestPacket(const uint8 packetType, PacketEx& packetAddTo, PacketEx& packetIsIn);
	// Requests that the packet type packetType is added to packetAddTo.
	const bool RequestPacket(const uint8 packetType, PacketEx& packetAddTo);

private:
	//
	// Methods
	//

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	const bool AddPacket(const uint8 packetType, PacketEx& packet);

	void Receive(sf::Packet& packet, const bool safe);

	// Read and send System variables
	void SendSystem(PacketEx& safe, PacketEx& fast);
	// Read and send Input
	void SendInput(PacketEx& safe, PacketEx& fast);
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

	// Returns true if one or more frames have passed since lastFrame
	inline const bool isNewFrame() { return (thisFrame != lastFrame); }

	//
	// Members
	//

	// Used for frame synchronization.
	uint thisFrame, lastFrame;

	PlayerObject recvPlayer, sendPlayer;
	InputStruct recvInput, sendInput;

	// Used for comparison to determine what to send.
	MemStruct local;

	// Analog throttle timer.
	// Prevents it from spamming packets.
	uint64 analogTimer;

	// Toggles and things
	bool firstMenuEntry;
	bool wroteP2Start;
	bool splitToggled;
	bool teleported;

	// Set in ReceivePlayer to true upon receival of a valid player message.
	bool writePlayer;

	// Set in SendSystem on level change to true if playing a relevant character.
	// (Sonic, Shadow, Amy, Metalsonic)
	bool sendSpinTimer;
};
