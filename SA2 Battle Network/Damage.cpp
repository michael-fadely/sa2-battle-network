#include "stdafx.h"

#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Globals.h"
#include "AddressList.h"
#include "Damage.h"
#include "FunctionPointers.h"
#include "AddRings.h"

using namespace nethax;

static Trampoline* DamagePlayer_trampoline;
static Trampoline* KillPlayer_trampoline;
static bool called_damage = false;

#pragma region Originals

Sint32 events::DamagePlayer_original(CharObj1* data1, CharObj2Base* data2)
{
	_FunctionPointer(Sint32, target, (CharObj1*, CharObj2Base*), DamagePlayer_trampoline->Target());
	return target(data1, data2);
}

void events::KillPlayer_original(int playerNum)
{
	void* target = KillPlayer_trampoline->Target();
	__asm
	{
		push ebx
		mov ebx, playerNum
		call target
		pop ebx
	}
}

#pragma endregion

#pragma region Hooks

static Sint32 __cdecl DamagePlayer_cpp(CharObj1* data1, CharObj2Base* data2)
{
	if (!Globals::isConnected())
		return events::DamagePlayer_original(data1, data2);

	Sint32 result = 0;
	if (data2->PlayerNum != Globals::Broker->GetPlayerNumber())
		return result;

	// HACK:
	events::AddRings_SyncToggle(false);
	called_damage = true;

	if ((result = events::DamagePlayer_original(data1, data2)) != 0)
		Globals::Broker->Append(MessageID::P_Damage, Protocol::TCP, nullptr);

	// HACK:
	events::AddRings_SyncToggle(true);
	called_damage = false;

	return result;
}

static void __stdcall KillPlayer_cpp(int playerNum)
{
	if (!called_damage && Globals::isConnected())
	{
		if (playerNum != Globals::Broker->GetPlayerNumber())
			return;
		
		Globals::Broker->Append(MessageID::P_Kill, Protocol::TCP, nullptr, true);
	}

	events::KillPlayer_original(playerNum);
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

#pragma endregion

static bool MessageHandler(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	if (!roundStarted())
		return false;

	switch (type)
	{
		default:
			return false;

		case MessageID::P_Damage:
			// HACK:
			events::AddRings_SyncToggle(false);
			called_damage = true;

			MainCharObj1[pnum]->Status |= Status_Hurt;
			if (!events::DamagePlayer_original(MainCharObj1[pnum], MainCharObj2[pnum]))
				MainCharObj1[pnum]->Status &= ~Status_Unknown6;
			
			// HACK:
			events::AddRings_SyncToggle(true);
			called_damage = false;

			break;

		case MessageID::P_Kill:
			events::KillPlayer_original(pnum);
			break;
	}

	return true;
}

void events::InitDamage()
{
	DamagePlayer_trampoline = new Trampoline((size_t)DamagePlayer, 0x0047380A, DamagePlayer_cpp);
	KillPlayer_trampoline = new Trampoline((size_t)KillPlayerPtr, 0x0046B116, KillPlayer_asm);

	Globals::Broker->RegisterMessageHandler(MessageID::P_Damage, &MessageHandler);
	Globals::Broker->RegisterMessageHandler(MessageID::P_Kill, &MessageHandler);
}

void events::DeinitDamage()
{
	delete DamagePlayer_trampoline;
	delete KillPlayer_trampoline;
}
