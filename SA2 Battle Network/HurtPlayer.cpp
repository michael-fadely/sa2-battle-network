#include "stdafx.h"

#include <SA2ModLoader.h>
#include "Globals.h"
#include "HurtPlayer.h"

using namespace nethax;

DataPointer(void*, HurtPlayerFunc, 0x01A5A28C);

Sint32 __cdecl DamagePlayer_cpp(CharObj1* data1, CharObj2Base* data2);
void __cdecl HurtPlayer_cpp(int playerNum);
void __stdcall KillPlayer_cpp(int playerNum);
void KillPlayer_asm();

bool events::do_damage = false;
Trampoline events::DamagePlayerHax(0x00473800, 0x0047380A, DamagePlayer_cpp);
Trampoline events::HurtPlayerHax((size_t)HurtPlayer, 0x006C1AF6, HurtPlayer_cpp);
Trampoline events::KillPlayerHax((size_t)KillPlayerPtr, 0x0046B116, KillPlayer_asm);

Sint32 DamagePlayer_cpp(CharObj1* data1, CharObj2Base* data2)
{
	FunctionPointer(Sint32, original, (CharObj1*, CharObj2Base*), events::DamagePlayerHax.Target());

	if (!Globals::isConnected())
		return original(data1, data2);

	Sint32 result = 0;
	if (data2->PlayerNum != 0)
	{
		if (events::do_damage)
		{
			void* last = HurtPlayerFunc;
			HurtPlayerFunc = events::HurtPlayerHax.Target();

			data1->Status |= Status_Hurt;
			result = original(data1, data2);

			HurtPlayerFunc = last;
			events::do_damage = false;
		}
	}
	else if ((result = original(data1, data2)) != 0)
	{
		Globals::Broker->Request(MessageID::P_Damage, Protocol::TCP);
	}

	return result;
}

void __cdecl HurtPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Request(MessageID::P_Hurt, Protocol::TCP, true);
	}

	FunctionPointer(void, target, (int), events::HurtPlayerHax.Target());
	target(playerNum);
}

void __stdcall KillPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Request(MessageID::P_Kill, Protocol::TCP, true);
	}

	events::KillPlayerOriginal(playerNum);
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
