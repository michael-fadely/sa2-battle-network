#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Globals.h"
#include "HurtPlayer.h"
#include "FunctionPointers.h"

using namespace nethax;

DataPointer(void*, HurtPlayerFunc, 0x01A5A28C);

static Trampoline* DamagePlayerHax;
static Trampoline* HurtPlayerHax;
static Trampoline* KillPlayerHax;
static bool do_damage = false;

static Sint32 DamagePlayer_cpp(CharObj1* data1, CharObj2Base* data2)
{
	_FunctionPointer(Sint32, original, (CharObj1*, CharObj2Base*), DamagePlayerHax->Target());

	if (!Globals::isConnected())
		return original(data1, data2);

	Sint32 result = 0;
	if (data2->PlayerNum != 0)
	{
		if (do_damage)
		{
			void* last = HurtPlayerFunc;
			HurtPlayerFunc = HurtPlayerHax->Target();

			data1->Status |= Status_Hurt;
			result = original(data1, data2);

			HurtPlayerFunc = last;
			do_damage = false;
		}
	}
	else if ((result = original(data1, data2)) != 0)
	{
		Globals::Broker->Append(MessageID::P_Damage, Protocol::TCP, nullptr);
	}

	return result;
}

static void __cdecl HurtPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Append(MessageID::P_Hurt, Protocol::TCP, nullptr, true);
	}

	events::HurtPlayerOriginal(playerNum);
}

static void __stdcall KillPlayer_cpp(int playerNum)
{
	if (Globals::isConnected())
	{
		if (playerNum != 0)
			return;

		Globals::Broker->Append(MessageID::P_Kill, Protocol::TCP, nullptr, true);
	}

	events::KillPlayerOriginal(playerNum);
}

static void __declspec(naked) KillPlayer_asm()
{
	__asm
	{
		push ebx
		call KillPlayer_cpp
		ret
	}
}

void events::KillPlayerOriginal(int playerNum)
{
	void* target = KillPlayerHax->Target();
	__asm
	{
		push ebx
		mov ebx, playerNum
		call target
		pop ebx
	}
}

void events::HurtPlayerOriginal(int playerNum)
{
	_FunctionPointer(void, target, (int playerNum), HurtPlayerHax->Target());
	target(playerNum);
}

static bool MessageHandler(MessageID type, int pnum, sf::Packet& packet)
{
	if (!roundStarted())
		return false;

	switch (type)
	{
		default:
			return false;

		case MessageID::P_Damage:
			do_damage = true;
			break;

		case MessageID::P_Hurt:
			events::HurtPlayerOriginal(pnum);
			break;

		case MessageID::P_Kill:
			events::KillPlayerOriginal(pnum);
			break;
	}

	return true;
}

void events::InitHurtPlayer()
{
	DamagePlayerHax = new Trampoline(0x00473800, 0x0047380A, DamagePlayer_cpp);
	HurtPlayerHax = new Trampoline((size_t)HurtPlayer, 0x006C1AF6, HurtPlayer_cpp);
	KillPlayerHax = new Trampoline((size_t)KillPlayerPtr, 0x0046B116, KillPlayer_asm);

	Globals::Broker->RegisterMessageHandler(MessageID::P_Damage, &MessageHandler);
	Globals::Broker->RegisterMessageHandler(MessageID::P_Hurt, &MessageHandler);
	Globals::Broker->RegisterMessageHandler(MessageID::P_Kill, &MessageHandler);
}

void events::DeinitHurtPlayer()
{
	delete DamagePlayerHax;
	delete HurtPlayerHax;
	delete KillPlayerHax;
}
