#include "stdafx.h"

#include <Trampoline.h>
#include "Globals.h"
#include "OnResult.h"
#include "FunctionPointers.h"
#include "Networking.h"
#include "PacketOverloads.h"

using namespace nethax;
using namespace Globals;

static Trampoline* WinTrampoline    = nullptr;
static Trampoline* ResultTrampoline = nullptr;
static Trampoline* DispTrampoline   = nullptr;

#pragma pack(push, 1)
struct DispWinnerAndContinue_Data
{
	char Action;
	char Player;
	char Selection;
	char field_3;
	int Timer;
	int field_8;
	int field_C;
	int field_10;
};
#pragma pack(pop)


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
		packet << (PlayerNumber)player;
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
		packet << (PlayerNumber)player;
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

static bool disp_deleted = false;
static ObjectFuncPtr delete_sub = nullptr;
static DispWinnerAndContinue_Data local_data = {};
static DispWinnerAndContinue_Data remote_data = {};

static void __cdecl DispWinnerAndContinue_Delete_wrapper(ObjectMaster* obj)
{
	disp_deleted = true;
	delete_sub(obj);
	delete_sub = nullptr;
	remote_data = {};
	local_data = {};
}

static void __cdecl DispWinnerAndContinue_wrapper(ObjectMaster* _this)
{
	if (disp_deleted)
	{
		disp_deleted = false;
		return;
	}

	if (delete_sub == nullptr)
	{
		delete_sub = _this->DeleteSub;
		_this->DeleteSub = DispWinnerAndContinue_Delete_wrapper;
	}

	auto data = (DispWinnerAndContinue_Data*)_this->Data2;
	if (data == nullptr)
		return;

	_ObjectFunc(mainsub, DispTrampoline->Target());

	if (data->Action == 0)
	{
		mainsub(_this);
		return;
	}

	mainsub(_this);

	if (Networking->isServer())
	{
		if (disp_deleted)
		{
			disp_deleted = false;
			return;
		}

		if (local_data.Action != data->Action || local_data.Selection != data->Selection)
		{
			sf::Packet packet;
			packet << data->Action << data->Selection;
			Broker->Append(MessageID::S_WinData, Protocol::TCP, &packet);
		}

		local_data = *data;
	}
	else if (remote_data.Action != 0)
	{
		data->Action = remote_data.Action;
		data->Selection = remote_data.Selection;

		if (data->Action != 4 && data->Action != 5)
			data->Timer = 0;
	}
}

static bool MessageHandler(MessageID type, PlayerNumber pnum, sf::Packet& packet)
{
	switch (type)
	{
		default:
			return false;

		case MessageID::S_Win:
		{
			PlayerNumber player;
			packet >> player;
			OnWin_orig(player);
			return true;
		}

		case MessageID::S_Result:
		{
			PlayerNumber player;
			packet >> player;
			OnResult_orig(player);
			return true;
		}

		case MessageID::S_WinData:
		{
			packet >> remote_data.Action >> remote_data.Selection;
#ifdef _DEBUG
			PrintDebug("WIN DATA A/S: %d/%d", (int)remote_data.Action, (int)remote_data.Selection);
#endif
			return true;
		}
	}
}

void InitOnResult()
{
	Broker->RegisterMessageHandler(MessageID::S_Win, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::S_Result, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::S_WinData, MessageHandler);

	WinTrampoline    = new Trampoline(0x0043E6D0, 0x0043E6D5, OnWin_asm);
	ResultTrampoline = new Trampoline(0x00451450, 0x00451455, OnResult);
	DispTrampoline   = new Trampoline(0x00451050, 0x00451056, DispWinnerAndContinue_wrapper);
}

void DeinitOnResult()
{
	delete WinTrampoline;
	delete ResultTrampoline;
	delete DispTrampoline;
}
