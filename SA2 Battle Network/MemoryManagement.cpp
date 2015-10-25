#include "stdafx.h"

#include "typedefs.h"
#include "AddressList.h"
#include "LazyMemory.h"
#include "nop.h"
#include "MemoryManagement.h"

void MemManage::keepActive(const bool doNop)
{
	if (doNop)
		nop::apply(ADDR_WINDOWACTIVE, 15);
	else
		nop::restore(ADDR_WINDOWACTIVE);
}

void MemManage::nop2PSpecials(const bool doNop)
{
	if (doNop)
	{
		PrintDebug("<> Disabling specials...");
		nop::apply(0x00724257, 2);
		nop::apply(0x00736207, 2);
		nop::apply(0x00749917, 2);
	}
	else
	{
		PrintDebug("<> Enabling specials...");
		nop::restore(0x00724257);
		nop::restore(0x00736207);
		nop::restore(0x00749917);
	}
}

void MemManage::nopP2Input(const bool doNop)
{
	//waitInputInit();
	if (doNop)
	{
		nop::apply(ADDR_P2INOP, 6);
	}
	else
	{
		//waitFrame(5);
		nop::restore(ADDR_P2INOP);
	}
}

void MemManage::swapSpawn(const bool swapstart)
{
	char swap = (swapstart) ? 0x94 : 0x95;
	WriteMemory(0x43D9B1, &swap, sizeof(char));
}

void MemManage::swapCharsel(const bool swapcharsel)
{
	uint8 E4, E0;
	E4 = 0xE4;
	E0 = 0xE0;

	if (swapcharsel)
	{
		WriteMemory(0x66A632, &E4, sizeof(char));
		WriteMemory(0x66A637, &E0, sizeof(char));
		WriteMemory(0x66A670, &E0, sizeof(char));
		WriteMemory(0x66A687, &E4, sizeof(char));
	}
	else
	{
		WriteMemory(0x66A632, &E0, sizeof(char));
		WriteMemory(0x66A637, &E4, sizeof(char));
		WriteMemory(0x66A670, &E4, sizeof(char));
		WriteMemory(0x66A687, &E0, sizeof(char));
	}
}

void MemManage::swapInput(const bool doNop)
{
	PrintDebug("<> Swapping input devices...");

	if (doNop)
		nop::apply(0x00441BCA, 7);
	else
		nop::restore(0x00441BCA);

	ControllerData* p1ptr = ControllerPtr[0];
	ControllerData* p2ptr = ControllerPtr[1];

	PrintDebug("<> %08X %08X", p1ptr, p2ptr);

	ControllerPtr[0] = p2ptr;
	ControllerPtr[1] = p1ptr;

	PrintDebug("<> Swap complete.");
}
