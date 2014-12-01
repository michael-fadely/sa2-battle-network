#include <iostream>
#include <thread>
#include <chrono>

#include "Common.h"
#include "AddressList.h"
#include "LazyMemory.h"

#include "MemoryStruct.h"
#include "nop.h"
#include "MemoryManagement.h"

using namespace std;
using namespace chrono;


inline const uint MemManage::getElapsedFrames(const uint lastFrameCount) { return (FrameCount - lastFrameCount); }

const bool MemManage::elapsedFrames(const uint currentFrameCount, const uint frameCount)
{
	uint result = getElapsedFrames(currentFrameCount);

	if (result > frameCount)
		cout << "[elapsedFrames] Warning: Elapsed frames exceeded specified wait count. [" << result << " > " << frameCount << "]" << endl;

	return (result >= frameCount);
}

void MemManage::waitFrame(const uint frameCount, const uint lastFrame)
{
	uint frames = 0;
	uint last = 0;

	if (lastFrame == 0)
		last = FrameCount;
	else
		last = lastFrame;

	while (frames < frameCount)
	{
		frames = getElapsedFrames(last);
		SleepFor((milliseconds)1);
	}

	if (frames > frameCount)
		cout << "[waitFrame] Warning: Elapsed frames exceeded specified duration. [" << frames << " > " << frameCount << "]" << endl;

	return;
}

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
		cout << "<> Disabling specials..." << endl;
		nop::apply(0x00724257, 2);
		nop::apply(0x00736207, 2);
		nop::apply(0x00749917, 2);
	}
	else
	{
		cout << "<> Enabling specials..." << endl;
		nop::restore(0x00724257);
		nop::restore(0x00736207);
		nop::restore(0x00749917);
	}
}

void MemManage::nopP2Input(const bool doNop)
{
	waitInputInit();
	if (doNop)
	{
		nop::apply(ADDR_P2INOP, 6);
	}
	else
	{
		waitFrame(5);
		nop::restore(ADDR_P2INOP);
	}

	return;
}

void MemManage::swapSpawn(const bool swapstart)
{
	char swap = (swapstart) ? 0x94 : 0x95;
	WriteMemory(0x43D9B1, &swap, sizeof(char));
}

void MemManage::swapCharsel(const bool swapcharsel)
{
	uchar E4, E0;
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

// Returns true if both input structures have been initalized.
const bool MemManage::InputInitalized()
{
	return (P1InputPtr != nullptr && P2InputPtr != nullptr);
}

void MemManage::waitInputInit()
{
	if (!InputInitalized())
		cout << "Waiting for input structures to initialize..." << endl;

	while (!InputInitalized())
		SleepFor((milliseconds)1);

	return;
}

// TODO: Rewrite to use DataPointer from the SA2 Mod Loader instead of Read/WriteMemory
void MemManage::swapInput(const bool doNop)
{
	cout << "<> Swapping input devices..." << endl;

	if (doNop)
		nop::apply(0x00441BCA, 7);
	else
		nop::restore(0x00441BCA);

	waitInputInit();

	uint32 p1ptr = 0;
	uint32 p2ptr = 0;

	ReadMemory(ADDR_P1INPUT, &p1ptr, sizeof(int));
	ReadMemory(ADDR_P2INPUT, &p2ptr, sizeof(int));

	cout << hex << "<> " << p1ptr << " " << p2ptr << dec << endl;

	WriteMemory(ADDR_P1INPUT, &p2ptr, sizeof(int));
	WriteMemory(ADDR_P2INPUT, &p1ptr, sizeof(int));

	cout << "<> Swap complete." << endl;
}
