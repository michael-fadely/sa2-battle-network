#pragma once

#include "MemoryManagement.h"
#include "PlayerObject.h"
#include "InputStruct.h"
#include "MemoryStruct.h"

class QSocket;
class PacketHandler;

class MemoryHandler
{
	public:
		MemoryHandler(PacketHandler* packetHandler, bool isserver);
		~MemoryHandler();

		// Methods
		// Update remote variables
		void Read();

		// Read and send System variables
		void SendSystem(QSocket* Socket);
		// Read and send Input
		void SendInput(QSocket* Socket, uint sendTimer);
		// Read and send Player varaibles
		void SendPlayer(QSocket* Socket);
		// Read and send Menu variables
		void SendMenu(QSocket* Socket);

		// Receive and write Input
		void ReceiveInput(QSocket* Socket, uchar type);
		// Receive game/system variables
		void ReceiveSystem(QSocket* Socket, uchar type);
		// Receive and write Player variables
		void ReceivePlayer(QSocket* Socket, uchar type);
		// Receive and write Menu variables
		void ReceiveMenu(QSocket* Socket, uchar type);

		void PreReceive();
		void PostReceive();

		// Reads the frame count from memory into thisFrame
		inline void GetFrame() { thisFrame = MemManage::getFrameCount(); }
		// Sets lastFrame to thisFrame
		inline void SetFrame() { lastFrame = thisFrame; }
		// Returns true if thisFrame is the same as lastFrame
		inline const bool CheckFrame() 
		{
			if (thisFrame == lastFrame)
				return true;
			else
				return false;
		}

		inline void setStartTime(uint time) { StartTime = time; }

	private:
		// Methods
		void InitPlayers();
		void InitInput();

		void DeinitPlayers();
		void DeinitInput();

		void writeP2Memory();
		void writeRings();
		void writeSpecials();
		void writeTimeStop();

		void updateAbstractPlayer(AbstractPlayer* recvr, PlayerObject* player);
		void ToggleSplitscreen();
		bool CheckTeleport();

		// Members
		uint thisFrame, lastFrame;

		PacketHandler* packetHandler;

		PlayerObject* player1;
		PlayerObject* player2;

		InputStruct* p1Input;
		InputStruct* p2Input;

		AbstractPlayer	recvPlayer, sendPlayer;
		abstractInput	recvInput, sendInput;
		
		// A Memory Structure that is "remote"
		// i.e it is constantly being updated with
		// content read from the game memory.
		MemStruct remote;

		// A Memory Structure that is "local"
		// i.e used for comparison for determining
		// what and what not to send.
		MemStruct local;

		// Timers etc
		uint analogTimer;

		// Things inherited from main program
		// No need for direct access
		uint StartTime;
		bool isServer;


		// Toggles
		bool firstMenuEntry;
		bool wroteP2Start;
		bool splitToggled;
		bool Teleported;
		bool writePlayer;
};
