#pragma once

#include <LazyTypedefs.h>

struct MemStruct;

// Functions
namespace MemManage
{
	/*
	// Realtime
	*/

	// Returns the number of elapsed frames since lastFrame
	uint32 getElapsedFrames(const uint32 lastFrame);

	// To be run on own thread - determines framerate
	void getFrameRate();

	// Non-Blocking
	// Returns true if frameCount frames has passed since lastFrame,
	// otherwise false
	bool elapsedFrames(const uint32 lastFrame, const uint32 frameCount = 1);
	// Blocking
	// Waits for frameCount frames to pass (since lastFrame, if not 0) before continuing
	//void waitFrame(const uint32 frameCount = 1, const uint32 lastFrame = 0);

	// Returns true if both input structures have been initalized.
	bool InputInitalized();
	// Blocking
	// Wait for input to be initialized before continuing
	//void waitInputInit();

	/*
	// Static
	*/

	void nop(const uint32 baseAddress, const uint32 size);
	void keepActive(const bool doNop);
	void nop2PSpecials(const bool doNop);
	void nopP2Input(const bool doNop);
	void swapSpawn(const bool swapstart);
	void swapCharsel(const bool swapcharsel);
	void swapInput(const bool doNop);
};