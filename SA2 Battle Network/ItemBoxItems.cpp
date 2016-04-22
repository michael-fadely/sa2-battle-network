#include "stdafx.h"
#include <SA2ModLoader.h>
#include "Globals.h"
#include "ItemBoxItems.h"

using namespace nethax;

static Trampoline* Speedup_Trampoline;
static Trampoline* NBarrier_Trampoline;
static Trampoline* TBarrier_Trampoline;
static Trampoline* Invincibility_Trampoline;

inline void ItemBox_Original(Trampoline* trampoline, ObjectMaster* object, int pnum)
{
	((decltype(ItemBoxItem::Code))trampoline->Target())(object, pnum);
}

void events::NBarrier_original(ObjectMaster* object, int pnum)
{
	ItemBox_Original(NBarrier_Trampoline, object, pnum);
}
void events::Speedup_original(ObjectMaster* object, int pnum)
{
	ItemBox_Original(Speedup_Trampoline, object, pnum);
}
void events::TBarrier_original(ObjectMaster* object, int pnum)
{
	ItemBox_Original(TBarrier_Trampoline, object, pnum);
}
void events::Invincibility_original(ObjectMaster* object, int pnum)
{
	ItemBox_Original(Invincibility_Trampoline, object, pnum);
}

static void __cdecl NBarrier_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_NBarrier, Protocol::TCP);

	events::NBarrier_original(obj, n);
}
static void __cdecl Speedup_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_Speedup, Protocol::TCP);

	events::Speedup_original(obj, n);
}
static void __cdecl TBarrier_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_TBarrier, Protocol::TCP);

	events::TBarrier_original(obj, n);
}
static void __cdecl Invincibility_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_Invincibility, Protocol::TCP);

	events::Invincibility_original(obj, n);
}

void events::InitItemBoxItems()
{
	NBarrier_Trampoline      = new Trampoline(0x006C8A40, 0x006C8A45, NBarrier_hax);
	Speedup_Trampoline       = new Trampoline(0x006C9870, 0x006C9875, Speedup_hax);
	TBarrier_Trampoline      = new Trampoline(0x006C98D0, 0x006C98D5, TBarrier_hax);
	Invincibility_Trampoline = new Trampoline(0x006C98F0, 0x006C98F5, Invincibility_hax);
}

void events::DeinitItemBoxItems()
{
	delete NBarrier_Trampoline;
	delete Speedup_Trampoline;
	delete TBarrier_Trampoline;
	delete Invincibility_Trampoline;
}
