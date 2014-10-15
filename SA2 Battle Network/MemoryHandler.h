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
	// Returns true if thisFrame is the same as lastFrame
	inline const bool CheckFrame() { return (thisFrame == lastFrame); }
	// Returns the current menu. Designed exclusively to be used externally.
	// This function does frame synchronization to ensure you don't catch the value
	// mid-operation. Used by class Program
	const unsigned int GetCurrentMenu();

	// Requests that the packet type packetType is added to packetAddTo if it is not present in packetIsIn
	const bool RequestPacket(const uchar packetType, PacketEx& packetAddTo, PacketEx& packetIsIn);
	// Requests that the packet type packetType is added to packetAddTo.
	const bool RequestPacket(const uchar packetType, PacketEx& packetAddTo);

private:
	//
	// Methods
	//

	// Called by RequestPacket
	// Adds the packet template for packetType to packet
	const bool AddPacket(const uchar packetType, PacketEx& packet);

	void Receive(sf::Packet& packet, const bool safe);

	// Read and send System variables
	void SendSystem();
	// Read and send Input
	void SendInput();
	// Read and send Player varaibles
	void SendPlayer();
	// Read and send Menu variables
	void SendMenu();

	// Receive and write Input
	bool ReceiveInput(uchar type, sf::Packet& packet);
	// Receive game/system variables
	bool ReceiveSystem(uchar type, sf::Packet& packet);
	// Receive and queue write of Player variables
	bool ReceivePlayer(uchar type, sf::Packet& packet);
	// Receive and write Menu variables
	bool ReceiveMenu(uchar type, sf::Packet& packet);

	void PreReceive();
	void PostReceive();

	// Pretty much all of these are just so I can be lazy
	void writeP2Memory();
	void writeRings();
	void writeSpecials();
	void writeTimeStop();

	// Populates a local player object (destination) with data from an ingame player (source).
	void UpdateAbstractPlayer(PlayerObject* destination, ObjectMaster* source);
	void ToggleSplitscreen();
	bool CheckTeleport();

	//
	// Members
	//

	// Used for frame synchronization.
	uint thisFrame, lastFrame;

	PlayerObject	recvPlayer, sendPlayer;
	InputStruct		recvInput, sendInput;

	// Used for comparison to determine what to send.
	MemStruct local;

	// Analog throttle timer.
	// Prevents it from spamming packets.
	uint analogTimer;

	// Used to determine whether or not Player 1 ([0]) and/or Player 2 ([1])
	// are at the 2P Battle Menu. I'm really not sure if I should do this some other way.
	bool cAt2PMenu[2];
	bool lAt2PMenu[2];

	// Toggles and things
	bool firstMenuEntry;
	bool wroteP2Start;
	bool splitToggled;
	bool Teleported;
	bool writePlayer;
	bool sendSpinTimer;
};
