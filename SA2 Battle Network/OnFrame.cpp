#include <LazyTypedefs.h>
#include <SA2ModLoader.h>

#include "Globals.h"

#include "OnFrame.h"

void* OnFrame_Ptr = (void*)0x004340E7;
void __declspec(naked) OnFrame()
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
	WriteData(OnFrame_Ptr, buffer, 9);
	WriteCall(OnFrame_Ptr, OnFrame);
	delete[] buffer;
}
