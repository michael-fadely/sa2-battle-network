#include "stdafx.h"
#include <SA2ModLoader.h>
#include <Trampoline.h>
#include "Globals.h"
#include "ItemBoxItems.h"

using namespace nethax;

static Trampoline* Speedup_Trampoline;
static Trampoline* NBarrier_Trampoline;
static Trampoline* TBarrier_Trampoline;
static Trampoline* Invincibility_Trampoline;

inline void __cdecl ItemBox_Original(Trampoline* trampoline, ObjectMaster* object, int pnum)
{
	((decltype(ItemBoxItem::Code))trampoline->Target())(object, pnum);
}

inline void __cdecl UserCallOriginal(void* original, int pnum)
{
	__asm
	{
		mov eax, [pnum]
		call [original]
	}
}

void __stdcall events::NBarrier_original(int pnum)
{
	UserCallOriginal(NBarrier_Trampoline->Target(), pnum);
}
void __stdcall events::Speedup_original(int pnum)
{
	UserCallOriginal(Speedup_Trampoline->Target(), pnum);
}
void __stdcall events::TBarrier_original(int pnum)
{
	UserCallOriginal(TBarrier_Trampoline->Target(), pnum);
}
void __stdcall events::Invincibility_original(ObjectMaster* object, int pnum)
{
	ItemBox_Original(Invincibility_Trampoline, object, pnum);
}

static void __cdecl NBarrier_hax(int n)
{
	if (Globals::isConnected())
	{
		if (n != 0)
			return;

		Globals::Broker->Request(MessageID::S_NBarrier, Protocol::TCP);
	}

	events::NBarrier_original(n);
}
static void __declspec(naked) NBarrier_asm()
{
	__asm
	{
		push eax
		call NBarrier_hax
		pop eax
		retn
	}
}

static void __cdecl Speedup_hax(int n)
{
	if (Globals::isConnected())
	{
		if (n != 0)
			return;

		Globals::Broker->Request(MessageID::S_Speedup, Protocol::TCP);
	}

	events::Speedup_original(n);
}
static void __declspec(naked) Speedup_asm()
{
	__asm
	{
		push eax
		call Speedup_hax
		pop eax
		retn
	}
}

static void __cdecl TBarrier_hax(int n)
{
	if (Globals::isConnected())
	{
		if (n != 0)
			return;

		Globals::Broker->Request(MessageID::S_TBarrier, Protocol::TCP);
	}

	events::TBarrier_original(n);
}
static void __declspec(naked) TBarrier_asm()
{
	__asm
	{
		push eax
		call TBarrier_hax
		pop eax
		retn
	}
}

static void __cdecl Invincibility_hax(ObjectMaster* obj, int n)
{
	if (Globals::isConnected())
	{
		if (n != 0)
			return;

		Globals::Broker->Request(MessageID::S_Invincibility, Protocol::TCP);
	}

	events::Invincibility_original(obj, n);
}

void events::InitItemBoxItems()
{
	NBarrier_Trampoline      = new Trampoline(0x0046E2E0, 0x0046E2FE, NBarrier_asm);
	Speedup_Trampoline       = new Trampoline(0x0046E120, 0x0046E126, Speedup_asm);
	TBarrier_Trampoline      = new Trampoline(0x0046E180, 0x0046E19E, TBarrier_asm);
	Invincibility_Trampoline = new Trampoline(0x006C98F0, 0x006C98F5, Invincibility_hax);
}

void events::DeinitItemBoxItems()
{
	delete NBarrier_Trampoline;
	delete Speedup_Trampoline;
	delete TBarrier_Trampoline;
	delete Invincibility_Trampoline;
}
