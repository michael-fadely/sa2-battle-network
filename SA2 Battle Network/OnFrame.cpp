#include <SA2ModLoader.h>

#include "Globals.h"
#include "AddressList.h"
#include "MemoryHandler.h"
#include <SFML/Network.hpp>
#include "PacketExtensions.h"
#include "Networking.h"

#include "OnFrame.h"

using namespace sa2bn;

void* caseDefault_ptr = (void*)0x004340CC;
void* case08_ptr = (void*)0x0043405D;
void* case09_ptr = (void*)0x0043407E;
void* case10_ptr = (void*)0x0043407E;

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

unsigned int lastFrame = 0;
void FrameHandler()
{
	if ((FrameCount - lastFrame) > FrameIncrement || FrameCount == lastFrame)
		PrintDebug("\a[FRAME DISCREPENCY]");
	
	lastFrame = FrameCount;

	if (!Globals::isConnected())
	{
		Globals::Program->Connect();
		return;
	}
	else if (!Globals::Program->CheckConnectOK())
	{
		Globals::Program->Disconnect(false, Program::ErrorCode::NotReady);
		return;
	}

	Globals::Memory->RecvLoop();
	Globals::Memory->GetFrame();

	PacketEx safe(true), fast(false);

	Globals::Memory->SendSystem(safe, fast);
	Globals::Memory->SendPlayer(safe, fast);
	Globals::Memory->SendMenu(safe, fast);

	Globals::Networking->Send(safe);
	Globals::Networking->Send(fast);

	Globals::Memory->SetFrame();
}

void InitOnFrame()
{
	WriteJump(case08_ptr, OnFrame_MidJump);
	WriteJump(case09_ptr, OnFrame_MidJump);
	WriteJump(case10_ptr, OnFrame_MidJump);

	// OnFrame caseDefault
	// Occurs if the current game mode isn't 8, 9 or 10, and byte_174AFF9 == 1
	WriteJump(caseDefault_ptr, OnFrame_MidJump);

	// OnFrame OnFrame_Hook
	// Occurs at the end of the function (effectively the "else" to the statement above)
	WriteJump(OnFrame_Hook_ptr, OnFrame_Hook);
}
