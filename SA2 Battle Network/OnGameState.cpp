#include <vector>			// obvious
#include <thread>			// for this_thread::yield

#include <SA2ModLoader.h>	// for everything
#include "CommonEnums.h"	// for GameState enum
#include "Networking.h"		// for MsgTypes
#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

DataPointer(uint, dword_174B058, 0x174B058);
void* escape_addr = (void*)0x0043AAEE;

void InitOnGameState()
{
	// TODO: Make revertable
	// Adding nops first because the existing instruction smaller than a call.
	std::vector<uint8> patch(5, 0x90);
	WriteData(escape_addr, patch.data(), patch.size());
	WriteCall(escape_addr, OnGameState);
}

static void __cdecl OnGameState()
{
	// This is here because we overwrite its assignment with a call
	// in the original code.
	dword_174B058 = 0;
	StageLoaded();
}

void StageLoaded()
{
	using namespace sa2bn::Globals;

	if (!isInitialized() || !isConnected())
		return;

	sf::Packet packet;
	packet << (uint8)MSG_READY;
	Networking->sendSafe(packet);

	if (!Broker->isClientReady)
	{
		PrintDebug("<> Waiting for players...");
		
		do
		{
			Broker->RecvLoop();
			std::this_thread::yield();
		} while (!Broker->isClientReady);

		PrintDebug(">> All players ready. Resuming game.");
	}
	
	Broker->isClientReady = false;
}
