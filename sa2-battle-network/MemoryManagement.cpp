#include "stdafx.h"

#include "AddressList.h"
#include "Nop.h"
#include "MemoryManagement.h"

static bool specials = false;

void mem_manage::nop_specials(const bool apply)
{
	if (specials == apply)
	{
		return;
	}

	if (apply)
	{
		PrintDebug("[SA2:BN] Disabling specials...");
		Nop::apply(0x00724257, 2);
		Nop::apply(0x00736207, 2);
		Nop::apply(0x00749917, 2);
	}
	else
	{
		PrintDebug("[SA2:BN] Enabling specials...");
		Nop::restore(0x00724257);
		Nop::restore(0x00736207);
		Nop::restore(0x00749917);
	}

	specials = apply;
}
