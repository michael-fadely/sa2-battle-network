#pragma once

struct MemStruct;

// Functions
namespace MemManage
{
	/*
	// Realtime
	*/

	// Returns the number of elapsed frames since lastFrame
	const unsigned int getElapsedFrames(const unsigned int lastFrame);

	// Gets current framecount from game memory
	const unsigned int getFrameCount();
	// To be run on own thread - determines framerate
	void getFrameRate();

	// Non-Blocking
	// Returns true if frameCount frames has passed since lastFrame,
	// otherwise false
	const bool elapsedFrames(const unsigned int lastFrame, const unsigned int frameCount = 1);
	// Blocking
	// Waits for frameCount frames to pass (since lastFrame, if not 0) before continuing
	void waitFrame(const unsigned int frameCount=1, const unsigned int lastFrame=0);

	// Returns true if both input structures have been initalized.
	const bool InputInitalized();
	// Blocking
	// Wait for input to be initialized before continuing
	void waitInputInit();

	/*
	// Static
	*/

	void nop(const unsigned int baseAddress, const unsigned int size);
	void keepActive();
	void nop2PSpecials();
	void nopP2Input(const bool doNop=true);
	void swapSpawn(const bool swapstart);
	void swapCharsel(const bool swapcharsel);
	void swapInput(const bool doNop=true);

	void changeGameState(unsigned char stateNum, MemStruct* structure);
};