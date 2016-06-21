#include "stdafx.h"
#include <Trampoline.h>
#include "Globals.h"
#include "OnResult.h"
#include "FunctionPointers.h"

using namespace nethax;
using namespace Globals;

static Trampoline* WinTrampoline = nullptr;
static Trampoline* ResultTrampoline = nullptr;

static void __cdecl OnResult_orig(Sint32 player)
{
	_FunctionPointer(void, original, (Sint32), ResultTrampoline->Target());
	original(player);
}

static void __cdecl OnResult(Sint32 player)
{
	if (!isConnected())
		return;

	if (Networking->isServer())
	{
		sf::Packet packet;
		packet << (Uint8)(player == 0);
		Broker->Append(MessageID::S_Result, Protocol::TCP, &packet);
		OnResult_orig(player);
	}
}

static void OnWin_orig(Sint32 player)
{
	auto target = WinTrampoline->Target();
	__asm
	{
		mov esi, [player]
		call target
	}
}

static void __cdecl OnWin(Sint32 player)
{
	if (!isConnected())
		return;

	if (Networking->isServer())
	{
		sf::Packet packet;
		packet << (Uint8)(player == 0);
		Broker->Append(MessageID::S_Win, Protocol::TCP, &packet);
		OnWin_orig(player);
	}
}

static void __declspec(naked) OnWin_asm()
{
	__asm
	{
		push esi
		call OnWin
		pop esi
		retn
	}
}

static bool MessageHandler(MessageID type, int pnum, sf::Packet& packet)
{
	switch (type)
	{
		default:
			return false;

		case MessageID::S_Win:
		{
			Uint8 player;
			packet >> player;
			OnWin_orig(player);
			return true;
		}

		case MessageID::S_Result:
		{
			Uint8 player;
			packet >> player;
			OnResult_orig(player);
			return true;
		}
	}
}

void InitOnResult()
{
	Broker->RegisterMessageHandler(MessageID::S_Win, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::S_Result, MessageHandler);

	WinTrampoline = new Trampoline(0x0043E6D0, 0x0043E6D5, OnWin_asm);
	ResultTrampoline = new Trampoline(0x00451450, 0x00451455, OnResult);
}

void DeinitOnResult()
{
	delete WinTrampoline;
	delete ResultTrampoline;
}
