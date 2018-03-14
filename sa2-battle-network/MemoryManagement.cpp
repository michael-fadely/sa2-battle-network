#include "stdafx.h"

#include "AddressList.h"
#include "nop.h"
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
		nop::apply(0x00724257, 2);
		nop::apply(0x00736207, 2);
		nop::apply(0x00749917, 2);
	}
	else
	{
		PrintDebug("[SA2:BN] Enabling specials...");
		nop::restore(0x00724257);
		nop::restore(0x00736207);
		nop::restore(0x00749917);
	}

	specials = apply;
}
