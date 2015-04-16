#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MSG
#include "Globals.h"		// for Globals :specialed:

#include "OnGameState.h"

DataPointer(uint8, byte_93BC5A, 0x93BC5A);

void* escape_addr = (void*)0x0043AB1E;

void InitOnGameState()
{
	// Adding nops first because the existing instruction is 6 bytes,
	// but the call we're adding is 5 bytes.
	uint8 patch[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	WriteData(escape_addr, arrayptrandlength(patch));
	WriteCall(escape_addr, OnGameState);
}

static void __cdecl OnGameState()
{
	// This is here because we overwrite its assignment with a call
	// in the original code.
	byte_93BC5A = 0;
	OnGameState_Ingame();
}

void OnGameState_Ingame()
{
	PrintDebug("INCOMING LAG");
}
