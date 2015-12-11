#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Globals.h"
#include "HurtPlayerTest.h"

using namespace nethax;

void __cdecl HurtPlayer_cpp(int playerNum);
void __stdcall KillPlayer_cpp(int playerNum);
void KillPlayer_asm();

Trampoline HurtPlayerHax((size_t)HurtPlayer, 0x006C1AF6, (DetourFunction)HurtPlayer_cpp);
Trampoline KillPlayerHax((size_t)KillPlayerPtr, 0x0046B116, KillPlayer_asm);

void __cdecl HurtPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Request(MessageID::P_Hurt, true, true);
	}

	FunctionPointer(void, target, (int playerNum), HurtPlayerHax.Target());
	target(playerNum);
}

void __stdcall KillPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Request(MessageID::P_Kill, true, true);
	}

	KillPlayerOriginal(playerNum);
}

void __declspec(naked) KillPlayer_asm()
{
	__asm
	{
		push ebx
		call KillPlayer_cpp
		ret
	}
}
