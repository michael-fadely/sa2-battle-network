#include <SA2ModLoader.h>
#include "Globals.h"
#include "OnStageChange.h"

void* SetCurrentLevel_ptr = (void*)0x0043D8A0;

void InitOnStageChange()
{
	WriteJump(SetCurrentLevel_ptr, SetCurrentLevel_asm);
}

int __declspec(naked) SetCurrentLevel_asm(int stage)
{
	__asm
	{
		push eax
		call SetCurrentLevel
		pop eax
		retn
	}
}

DataPointer(short, word_1934B84, 0x01934B84);
DataPointer(short, isFirstStageLoad, 0x01748B94);

void SetCurrentLevel(int stage)
{
	word_1934B84 = CurrentLevel;

	if (isFirstStageLoad)
		isFirstStageLoad = 0;

	CurrentLevel = stage;

	OnStageChange();
}

using namespace sa2bn;

void OnStageChange()
{
	using namespace sa2bn::Globals;

	if (!isInitialized())
		return;

	if (!isConnected())
		return;

	if (Networking->isServer())
	{
		PrintDebug("Stage load confirmation as server.");
	}
	else
	{
		PrintDebug("Stage load confirmation as client.");
	}
}