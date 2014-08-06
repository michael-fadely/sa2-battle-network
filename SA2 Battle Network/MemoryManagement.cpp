#include <iostream>
#include <thread>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"
#include "AddressList.h"
#include "LazyMemory.h"

#include "MemoryStruct.h"
#include "MemoryManagement.h"

using namespace std;
using namespace chrono;


inline const uint MemManage::getElapsedFrames(const uint lastFrameCount) { return (FrameCount - lastFrameCount); }

inline const uint MemManage::getFrameCount()
{
	return FrameCount;
}

const bool MemManage::elapsedFrames(const uint currentFrameCount, const uint frameCount)
{
	uint result = getElapsedFrames(currentFrameCount);
	
	if (result > frameCount)
		cout << "[elapsedFrames] Warning: Elapsed frames exceeded specified wait count. [" << result << " > " << frameCount << "]" << endl;

	return (result >= frameCount);
}

void MemManage::waitFrame(const uint frameCount, const uint lastFrame)
{
	uint frames	= 0;
	uint last = 0;

	if (lastFrame == 0)
		last = getFrameCount();
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

void MemManage::nop(const uint baseAddress, const uint size)
{
	char* nop = new char[size];
	memset(nop, 0x90, size);
	WriteMemory(baseAddress, nop, size);	
	delete[] nop;
}

void MemManage::keepActive()
{
	nop(ADDR_WINDOWACTIVE, 15);
	return;
}

void MemManage::nop2PSpecials()
{
	cout << "<> Disabling specials..." << endl;
	nop(0x00724257, 2);
	nop(0x00736207, 2);
	nop(0x00749917, 2);
}

void MemManage::nopP2Input(const bool doNop)
{
	if (doNop)
	{
		waitInputInit();
		nop(ADDR_P2INOP, 6);
	}
	else
	{
		waitInputInit();

		uchar buffer[6] = {
			0x0F,
			0x8C,
			0x16,
			0xFF,
			0xFF,
			0xFF
		};

		WriteMemory(ADDR_P2INOP, &buffer, (sizeof(char)*6));
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

void MemManage::swapInput(const bool doNop)
{
	cout << "<> Swapping input devices..." << endl;
	
	if (doNop)
		nop(0x00441BCA, 7);
	else
	{
		uchar buffer[7] = {
			0x89,
			0x04,
			0xB5,
			0x60,
			0xFB,
			0xDE,
			0x01
		};

		WriteMemory(0x00441BCA, &buffer, (sizeof(char)*7));
	}


	waitInputInit();

	uint p1 = 0;
	uint p2 = 0;

	ReadMemory(ADDR_P1INPUT, &p1, sizeof(int));
	ReadMemory(ADDR_P2INPUT, &p2, sizeof(int));

	cout << hex << "<> " << p1 << " " << p2 << dec << endl;

	WriteMemory(ADDR_P1INPUT, &p2, sizeof(int));
	WriteMemory(ADDR_P2INPUT, &p1, sizeof(int));

	cout << "<> Swap complete." << endl;
}

// Returns true if both input structures have been initalized.
const bool MemManage::InputInitalized()
{
	uint p1 = 0;
	uint p2 = 0;

	ReadMemory(ADDR_P1INPUT, &p1, sizeof(int));
	ReadMemory(ADDR_P2INPUT, &p2, sizeof(int));

	return (p1 != 0 && p2 != 0);
}

void MemManage::waitInputInit()
{
	if (!InputInitalized())
		cout << "Waiting for input structures to initialize..." << endl;

	while (!InputInitalized())
		SleepFor((milliseconds)1);

	return;
}