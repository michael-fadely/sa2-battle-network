#include "stdafx.h"
#include <SA2ModLoader.h>
#include "Globals.h"
#include "ItemBoxItems.h"

using namespace nethax;

ItemBoxItem events::Speedup_original = {};
ItemBoxItem events::NBarrier_original = {};
ItemBoxItem events::TBarrier_original = {};
ItemBoxItem events::Invincibility_original = {};

inline void do_things(int n, ItemBoxItem* item, void(__cdecl *func)(ObjectMaster *, int))
{
	*item = ItemBoxItems_A[n];
	ItemBoxItems_A[n].Code = func;
	ItemBoxItems_B[n].Code = func;
	ItemBoxItems_C[n].Code = func;
	ItemBoxItems_D[n].Code = func;
}

inline void undo_things(int n, ItemBoxItem* item)
{
	ItemBoxItems_A[n] = *item;
	ItemBoxItems_B[n] = *item;
	ItemBoxItems_C[n] = *item;
	ItemBoxItems_D[n] = *item;
}

static void __cdecl NBarrier_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_NBarrier, Protocol::TCP);

	events::NBarrier_original.Code(obj, n);
}

static void __cdecl Speedup_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_Speedup, Protocol::TCP);

	events::Speedup_original.Code(obj, n);
}

static void __cdecl TBarrier_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_TBarrier, Protocol::TCP);

	events::TBarrier_original.Code(obj, n);
}

static void __cdecl Invincibility_hax(ObjectMaster* obj, int n)
{
	if (n != 0)
		return;

	if (Globals::isConnected())
		Globals::Broker->Request(MessageID::S_Invincibility, Protocol::TCP);

	events::Invincibility_original.Code(obj, n);
}

void events::InitItemBoxItems()
{
	do_things(0, &Speedup_original, Speedup_hax);
	do_things(5, &NBarrier_original, NBarrier_hax);
	do_things(8, &TBarrier_original, TBarrier_hax);
	do_things(10, &Invincibility_original, Invincibility_hax);
}

void events::DeinitItemBoxItems()
{
	undo_things(0, &Speedup_original);
	undo_things(5, &NBarrier_original);
	undo_things(8, &TBarrier_original);
	undo_things(10, &Invincibility_original);
}
