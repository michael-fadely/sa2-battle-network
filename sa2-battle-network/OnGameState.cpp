#include "stdafx.h"

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MessageID
#include "globals.h"

#include "OnGameState.h"

DataPointer(uint, dword_174B058, 0x174B058);

// Note that the name is misleading. This only happens when the gamestate changes to Ingame.
// TODO: Real OnGameState
// TODO: Consider removing since PoseEffect2PStartMan is now synchronized
static void __stdcall OnGameState()
{
	using namespace nethax;
	using namespace globals;

	// This is here because we overwrite its assignment with a call
	// in the original code.
	dword_174B058 = 0;

	if (!is_connected())
	{
		return;
	}

	broker->send_ready_and_wait(MessageID::S_GameState);
}

// TODO: Make revertible
void nethax::events::InitOnGameState()
{
	WriteCall((void*)0x0043AAEE, OnGameState);
}
