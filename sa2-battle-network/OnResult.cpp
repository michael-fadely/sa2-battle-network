#include "stdafx.h"

#include <Trampoline.h>
#include "globals.h"
#include "OnResult.h"
#include "FunctionPointers.h"
#include "Networking.h"
#include "PacketOverloads.h"

using namespace nethax;
using namespace globals;

static Trampoline* win_trampoline    = nullptr;
static Trampoline* result_trampoline = nullptr;
static Trampoline* disp_trampoline   = nullptr;

enum DispAction : __int8
{
	DispAction_Initialize = 0x0,
	DispAction_Transition = 0x1,
	DispAction_YesNo      = 0x2,
	DispAction_Exit       = 0x3,
	DispAction_Continue   = 0x4,
	DispAction_Restart    = 0x5,
};

#pragma pack(push, 1)
struct DispWinnerAndContinue_Data
{
	int8_t Action;
	int8_t Player;
	int8_t Selection;
	int8_t field_3;
	int Timer;
	int field_8;
	int field_C;
	int field_10;
};
#pragma pack(pop)

static void __cdecl OnResult_orig(Sint32 player)
{
	_FunctionPointer(void, original, (Sint32), result_trampoline->Target());
	original(player);
}

static void __cdecl OnResult(Sint32 player)
{
	if (!is_connected())
	{
		return;
	}

	if (networking->is_server())
	{
		sws::Packet packet;
		packet << static_cast<pnum_t>(player);
		broker->append(MessageID::S_Result, Protocol::tcp, &packet);
		OnResult_orig(player);
	}
}

static void OnWin_orig(Sint32 player)
{
	auto target = win_trampoline->Target();
	__asm
	{
		mov esi, [player]
		call target
	}
}

static void __cdecl OnWin(Sint32 player)
{
	if (!is_connected())
	{
		return;
	}

	if (networking->is_server())
	{
		sws::Packet packet;
		packet << static_cast<pnum_t>(player);
		broker->append(MessageID::S_Win, Protocol::tcp, &packet);
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

	auto data = reinterpret_cast<DispWinnerAndContinue_Data*>(_this->Data2);

	if (data == nullptr)
	{
		return;
	}

	_ObjectFunc(mainsub, disp_trampoline->Target());

	if (data->Action == 0)
	{
		mainsub(_this);
		return;
	}

	mainsub(_this);

	if (networking->is_server())
	{
		if (disp_deleted)
		{
			disp_deleted = false;
			return;
		}

		if (local_data.Action != data->Action || local_data.Selection != data->Selection)
		{
			sws::Packet packet;
			packet << data->Action << data->Selection;
			broker->append(MessageID::S_WinData, Protocol::tcp, &packet);
		}

		local_data = *data;
	}
	else if (remote_data.Action != DispAction_Initialize)
	{
		data->Action = remote_data.Action;
		data->Selection = remote_data.Selection;

		if (data->Action != DispAction_Continue && data->Action != DispAction_Restart)
			data->Timer = 0;
	}
}

static bool MessageHandler(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	switch (type)
	{
		default:
			return false;

		case MessageID::S_Win:
		{
			pnum_t player;
			packet >> player;
			OnWin_orig(player);
			return true;
		}

		case MessageID::S_Result:
		{
			pnum_t player;
			packet >> player;
			OnResult_orig(player);
			return true;
		}

		case MessageID::S_WinData:
		{
			packet >> remote_data.Action >> remote_data.Selection;

#ifdef _DEBUG
			PrintDebug("WIN DATA A/S: %d/%d",
					   static_cast<int>(remote_data.Action),
					   static_cast<int>(remote_data.Selection));
#endif
			return true;
		}
	}
}

void InitOnResult()
{
	broker->register_message_handler(MessageID::S_Win, MessageHandler);
	broker->register_message_handler(MessageID::S_Result, MessageHandler);
	broker->register_message_handler(MessageID::S_WinData, MessageHandler);

	win_trampoline    = new Trampoline(0x0043E6D0, 0x0043E6D5, OnWin_asm);
	result_trampoline = new Trampoline(0x00451450, 0x00451455, OnResult);
	disp_trampoline   = new Trampoline(0x00451050, 0x00451056, DispWinnerAndContinue_wrapper);
}

void DeinitOnResult()
{
	delete win_trampoline;
	delete result_trampoline;
	delete disp_trampoline;
}
