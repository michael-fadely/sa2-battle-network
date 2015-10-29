#include "stdafx.h"

#include <vector>			// obvious

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MessageID
#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

DataPointer(uint, dword_174B058, 0x174B058);
void* escape_addr = (void*)0x0043AAEE;

// Note that the name is misleading. This only happens when the gamestate changes to Ingame.
// TODO: Real OnGameState
static void __cdecl OnGameState()
{
	using namespace nethax;
	using namespace Globals;

	// This is here because we overwrite its assignment with a call
	// in the original code.
	dword_174B058 = 0;

	if (!isInitialized() || !isConnected())
		return;

	Broker->SendReady(MessageID::S_GameState);
	Broker->WaitForPlayers(MessageID::S_GameState);
}

void InitOnGameState()
{
	// TODO: Make revertable
	// Adding nops first because the existing instruction smaller than a call.
	std::vector<uint8> patch(5, 0x90);
	WriteData(escape_addr, patch.data(), patch.size());
	WriteCall(escape_addr, OnGameState);
}
