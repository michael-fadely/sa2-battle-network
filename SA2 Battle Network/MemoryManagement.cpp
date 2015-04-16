
#include <thread>
#include <chrono>

#include "Common.h"
#include "AddressList.h"
#include "LazyMemory.h"

#include "nop.h"

#include "MemoryManagement.h"

using namespace std;
using namespace chrono;

inline uint32 MemManage::getElapsedFrames(const uint32 lastFrameCount) { return (FrameCount - lastFrameCount); }

bool MemManage::elapsedFrames(const uint32 currentFrameCount, const uint32 frameCount)
{
	uint32 result = getElapsedFrames(currentFrameCount);

	//if (result > frameCount)
	//	cout << "[elapsedFrames] Warning: Elapsed frames exceeded specified wait count. [" << result << " > " << frameCount << "]" << endl;

	return (result >= frameCount);
}

/*
void MemManage::waitFrame(const uint32 frameCount, const uint32 lastFrame)
{
	uint32 frames = 0;
	uint32 last = 0;

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
*/

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

	return;
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

// Returns true if both input structures have been initalized.
bool MemManage::InputInitalized()
{
	return (ControllerPtr1 != nullptr && ControllerPtr2 != nullptr);
}
/*
void MemManage::waitInputInit()
{
	if (!InputInitalized())
		PrintDebug("Waiting for input structures to initialize...");

	while (!InputInitalized())
		std::this_thread::yield();

	return;
}
*/

void MemManage::swapInput(const bool doNop)
{
	PrintDebug("<> Swapping input devices...");

	if (doNop)
		nop::apply(0x00441BCA, 7);
	else
		nop::restore(0x00441BCA);

	//waitInputInit();

	ControllerData* p1ptr = ControllerPtr1;
	ControllerData* p2ptr = ControllerPtr2;

	PrintDebug("<> %08X %08X", p1ptr, p2ptr);

	ControllerPtr1 = p2ptr;
	ControllerPtr2 = p1ptr;

	PrintDebug("<> Swap complete.");
}
