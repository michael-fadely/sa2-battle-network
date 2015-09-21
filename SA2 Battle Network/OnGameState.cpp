#include <vector>			// obvious

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MsgTypes
#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

DataPointer(uint, dword_174B058, 0x174B058);
void* escape_addr = (void*)0x0043AAEE;

static void __cdecl OnGameState()
{
	using namespace sa2bn;
	using namespace Globals;

	// This is here because we overwrite its assignment with a call
	// in the original code.
	dword_174B058 = 0;

	if (!isInitialized() || !isConnected())
		return;

	sf::Packet packet;
	packet << (uint8)Message::N_Ready;
	Networking->sendSafe(packet);

	Broker->WaitForPlayers(Broker->isClientReady);
}

void InitOnGameState()
{
	// TODO: Make revertable
	// Adding nops first because the existing instruction smaller than a call.
	std::vector<uint8> patch(5, 0x90);
	WriteData(escape_addr, patch.data(), patch.size());
	WriteCall(escape_addr, OnGameState);
}
