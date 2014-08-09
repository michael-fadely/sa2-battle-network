#pragma once

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

	// Methods

	void RecvLoop();
	void SendLoop();

	// Reads the frame count from memory into thisFrame
	inline void GetFrame() { thisFrame = FrameCount; }
	// Sets lastFrame to thisFrame
	inline void SetFrame() { lastFrame = thisFrame; }
	// Returns true if thisFrame is the same as lastFrame
	inline const bool CheckFrame() { return (thisFrame == lastFrame); }

private:
	// Methods

	void Receive(sf::Packet& packet, const bool safe);

	// Read and send System variables
	void SendSystem();
	// Read and send Input
	void SendInput(/*uint sendTimer*/);
	// Read and send Player varaibles
	void SendPlayer();
	// Read and send Menu variables
	void SendMenu();

	// Receive and write Input
	bool ReceiveInput(uchar type, sf::Packet& packet);
	// Receive game/system variables
	bool ReceiveSystem(uchar type, sf::Packet& packet);
	// Receive and write Player variables
	bool ReceivePlayer(uchar type, sf::Packet& packet);
	// Receive and write Menu variables
	bool ReceiveMenu(uchar type, sf::Packet& packet);

	void PreReceive();
	void PostReceive();

	void writeP2Memory();
	void writeRings();
	void writeSpecials();
	void writeTimeStop();

	void updateAbstractPlayer(PlayerObject* destination, ObjectMaster* source);
	void ToggleSplitscreen();
	bool CheckTeleport();

	// Members
	uint thisFrame, lastFrame;

	PlayerObject	recvPlayer, sendPlayer;
	InputStruct		recvInput, sendInput;

	// A Memory Structure that is "local".
	// Used for comparison to determine what to send.
	MemStruct local;

	// Timers etc
	uint analogTimer;

	// Used to determine whether or not Player 1 ([0]) and/or Player 2 ([1])
	// are at the 2P Battle Menu. I'm really not sure if I should do this some other way.
	bool cAt2PMenu[2];
	bool lAt2PMenu[2];

	// Toggles
	bool firstMenuEntry;
	bool wroteP2Start;
	bool splitToggled;
	bool Teleported;
	bool writePlayer;
};
