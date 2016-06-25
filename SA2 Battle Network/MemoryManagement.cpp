#include "stdafx.h"

#include "typedefs.h"
#include "AddressList.h"
#include "LazyMemory.h"
#include "nop.h"
#include "MemoryManagement.h"

static bool specials = false;
void MemManage::nop2PSpecials(const bool apply)
{
	if (specials == apply)
		return;

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

void MemManage::swapSpawn(const bool swapstart)
{
	return;

	uint8 swap = (swapstart) ? 0x94 : 0x95;
	WriteMemory(0x43D9B1, &swap, sizeof(uint8));
}

void MemManage::swapCharsel(const bool swapcharsel)
{
	return;

	uint8 E4 = 0xE4;
	uint8 E0 = 0xE0;

	if (swapcharsel)
	{
		WriteMemory(0x66A632, &E4, sizeof(uint8));
		WriteMemory(0x66A637, &E0, sizeof(uint8));
		WriteMemory(0x66A670, &E0, sizeof(uint8));
		WriteMemory(0x66A687, &E4, sizeof(uint8));
	}
	else
	{
		WriteMemory(0x66A632, &E0, sizeof(uint8));
		WriteMemory(0x66A637, &E4, sizeof(uint8));
		WriteMemory(0x66A670, &E4, sizeof(uint8));
		WriteMemory(0x66A687, &E0, sizeof(uint8));
	}
}

static bool input = false;
void MemManage::swapInput(const bool apply)
{
	return;

	if (input == apply)
		return;

	PrintDebug("[SA2:BN] Swapping input devices...");

	if (apply)
		nop::apply(0x00441BCA, 7);
	else
		nop::restore(0x00441BCA);

	ControllerData* p1ptr = ControllerPointers[0];
	ControllerData* p2ptr = ControllerPointers[1];

	PrintDebug("<> %08X %08X", p1ptr, p2ptr);

	ControllerPointers[0] = p2ptr;
	ControllerPointers[1] = p1ptr;

	input = apply;
}
