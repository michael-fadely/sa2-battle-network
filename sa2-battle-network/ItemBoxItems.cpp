#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Networking.h"
#include "globals.h"
#include "ItemBoxItems.h"
#include "FunctionPointers.h"

using namespace nethax;

static Trampoline* Speedup_trampoline;
static Trampoline* NBarrier_trampoline;
static Trampoline* TBarrier_trampoline;
static Trampoline* Invincibility_trampoline;
static Trampoline* DisplayItemBoxItem_trampoline;

inline void __cdecl ItemBox_original(Trampoline* trampoline, ObjectMaster* object, int pnum)
{
	reinterpret_cast<decltype(ItemBoxItem::Code)>(trampoline->Target())(object, pnum);
}

inline void __cdecl user_call_original(void* original, int pnum)
{
	__asm
	{
		mov eax, [pnum]
		call [original]
	}
}

void __stdcall events::NBarrier_original(int pnum)
{
	user_call_original(NBarrier_trampoline->Target(), pnum);
}

void __stdcall events::Speedup_original(int pnum)
{
	user_call_original(Speedup_trampoline->Target(), pnum);
}

void __stdcall events::TBarrier_original(int pnum)
{
	user_call_original(TBarrier_trampoline->Target(), pnum);
}

void __stdcall events::Invincibility_original(ObjectMaster* object, int pnum)
{
	ItemBox_original(Invincibility_trampoline, object, pnum);
}

void events::DisplayItemBoxItem_original(int pnum, int tnum)
{
	_FunctionPointer(void, original, (int, int), DisplayItemBoxItem_trampoline->Target());
	original(pnum, tnum);
}

static void __cdecl NBarrier_cpp(int n)
{
	if (globals::is_connected())
	{
		if (n != globals::broker->get_player_number())
		{
			return;
		}

		globals::broker->append(MessageID::S_NBarrier, Protocol::tcp, nullptr);
	}

	events::NBarrier_original(n);
}

static void __declspec(naked) NBarrier_asm()
{
	__asm
	{
		push eax
		call NBarrier_cpp
		pop eax
		retn
	}
}

static void __cdecl Speedup_cpp(int n)
{
	if (globals::is_connected())
	{
		if (n != globals::broker->get_player_number())
		{
			return;
		}

		globals::broker->append(MessageID::S_Speedup, Protocol::tcp, nullptr);
	}

	events::Speedup_original(n);
}

static void __declspec(naked) Speedup_asm()
{
	__asm
	{
		push eax
		call Speedup_cpp
		pop eax
		retn
	}
}

static void __cdecl TBarrier_cpp(int n)
{
	if (globals::is_connected())
	{
		if (n != globals::broker->get_player_number())
		{
			return;
		}

		globals::broker->append(MessageID::S_TBarrier, Protocol::tcp, nullptr);
	}

	events::TBarrier_original(n);
}

static void __declspec(naked) TBarrier_asm()
{
	__asm
	{
		push eax
		call TBarrier_cpp
		pop eax
		retn
	}
}

static void __cdecl Invincibility_cpp(ObjectMaster* obj, int n)
{
	if (globals::is_connected())
	{
		if (n != globals::broker->get_player_number())
		{
			return;
		}

		globals::broker->append(MessageID::S_Invincibility, Protocol::tcp, nullptr);
	}

	events::Invincibility_original(obj, n);
}

static void __cdecl DisplayItemBoxItem_cpp(int pnum, int tnum)
{
	if (globals::is_connected())
	{
		if (pnum != globals::broker->get_player_number())
		{
			return;
		}

		sws::Packet packet;
		packet << tnum;
		globals::broker->append(MessageID::S_ItemBoxItem, Protocol::tcp, &packet, true);
	}

	events::DisplayItemBoxItem_original(pnum, tnum);
}

static bool message_handler(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	if (!round_started())
	{
		return false;
	}

	switch (type)
	{
		case MessageID::S_NBarrier:
			events::NBarrier_original(pnum);
			break;

		case MessageID::S_TBarrier:
			events::TBarrier_original(pnum);
			break;

		case MessageID::S_Speedup:
			events::Speedup_original(pnum);
			break;

		case MessageID::S_Invincibility:
			events::Invincibility_original(nullptr, pnum);
			break;

		case MessageID::S_ItemBoxItem:
		{
			int tnum;
			packet >> tnum;
			events::DisplayItemBoxItem_original(pnum, tnum);
			break;
		}

		default:
			return false;
	}

	return true;
}

void events::InitItemBoxItems()
{
	NBarrier_trampoline = new Trampoline(0x0046E2E0, 0x0046E2FE, NBarrier_asm);
	Speedup_trampoline = new Trampoline(0x0046E120, 0x0046E126, Speedup_asm);
	TBarrier_trampoline = new Trampoline(0x0046E180, 0x0046E19E, TBarrier_asm);
	Invincibility_trampoline = new Trampoline(0x006C98F0, 0x006C98F5, Invincibility_cpp);
	DisplayItemBoxItem_trampoline = new Trampoline(0x006DF440, 0x006DF445, DisplayItemBoxItem_cpp);

	globals::broker->register_message_handler(MessageID::S_NBarrier, message_handler);
	globals::broker->register_message_handler(MessageID::S_TBarrier, message_handler);
	globals::broker->register_message_handler(MessageID::S_Speedup, message_handler);
	globals::broker->register_message_handler(MessageID::S_Invincibility, message_handler);
	globals::broker->register_message_handler(MessageID::S_ItemBoxItem, message_handler);
}

void events::DeinitItemBoxItems()
{
	delete NBarrier_trampoline;
	delete Speedup_trampoline;
	delete TBarrier_trampoline;
	delete Invincibility_trampoline;
	delete DisplayItemBoxItem_trampoline;
}
