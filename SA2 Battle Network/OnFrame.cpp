#include <SA2ModLoader.h>

#include "Globals.h"

#include "OnFrame.h"

void* caseDefault_ptr = (void*)0x004340CC;
void* caseRestartNoLife_ptr = (void*)0x0043407E;	// Doesn't appear to ever execute

void __declspec(naked) OnFrame_MidJump()
{
	__asm
	{
		push eax
		call FrameHandler
		pop eax

		pop edi
		pop esi
		pop ebp
		pop ebx
		pop ecx
		retn
	}
}

void* OnFrame_Hook_ptr = (void*)0x004340E7;
void __declspec(naked) OnFrame_Hook()
{
	__asm
	{
		push eax
		call FrameHandler
		pop eax
		retn
	}
}


void FrameHandler()
{
	PrintDebug("FrameHandler");
}

void InitOnFrame()
{
	char* buffer = new char[9];
	memset(buffer, 0xC3, 9);
	
	// OnFrame caseDefault
	// Occurs if the current gamestate isn't Exit1, RestartLevel_NoLifeLost,
	//  or Exit2, and byte_174AFF9 == 1
	WriteJump(caseDefault_ptr, OnFrame_MidJump);

	// OnFrame OnFrame_Hook
	// Occurs at the end of the function (effectively the "else" to the statement above)
	WriteData(OnFrame_Hook_ptr, buffer, 9);
	WriteJump(OnFrame_Hook_ptr, OnFrame_Hook);

	delete[] buffer;
}
