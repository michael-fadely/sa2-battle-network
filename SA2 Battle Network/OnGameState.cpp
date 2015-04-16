#include <vector>

#include <LazyTypedefs.h>
#include <SA2ModLoader.h>	// for everything
#include "CommonEnums.h"	// for GameState enum
#include "AddressList.h"	// for GameState, FrameCount
//#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

void* escape_addr = (void*)0x0043AAD2;

void InitOnGameState()
{
	// Adding nops first because the existing instruction smaller than a call.
	std::vector<uint8> patch(7, 0x90);
	WriteData(escape_addr, patch.data(), patch.size());
	WriteCall(escape_addr, OnGameState);
}

static void __cdecl OnGameState()
{
	// This is here because we overwrite its assignment with a call
	// in the original code.
	GameState = GameState::INGAME;
	OnGameState_Ingame();
}

void OnGameState_Ingame()
{
	PrintDebug("\a[%06d] Stage loaded.", FrameCount);
}
