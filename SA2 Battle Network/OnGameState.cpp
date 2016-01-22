#include "stdafx.h"

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MessageID
#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

DataPointer(uint, dword_174B058, 0x174B058);
void* escape_addr = (void*)0x0043AAEE;

// Note that the name is misleading. This only happens when the gamestate changes to Ingame.
// TODO: Real OnGameState
static void __stdcall OnGameState()
{
	using namespace nethax;
	using namespace Globals;

	// This is here because we overwrite its assignment with a call
	// in the original code.
	dword_174B058 = 0;

	if (!isConnected())
		return;

	Broker->SendReady(MessageID::S_GameState);
	Broker->WaitForPlayers(MessageID::S_GameState);
}

void InitOnGameState()
{
	// TODO: Make revertable
	WriteCall(escape_addr, OnGameState);
}
