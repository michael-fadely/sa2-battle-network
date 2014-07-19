#pragma once

struct MemStruct;

// Functions
namespace MemManage
{
	/*
	// Realtime
	*/

	// Returns the number of elapsed frames since lastFrame
	unsigned int getElapsedFrames(unsigned int lastFrame);

	// Gets current framecount from game memory
	unsigned int getFrameCount();
	// To be run on own thread - determines framerate
	void getFrameRate();

	// Non-Blocking
	// Returns true if frameCount frames has passed since lastFrame,
	// otherwise false
	bool waitFrameRecursive(unsigned int lastFrame, unsigned int frameCount=1);
	// Blocking
	// Waits for frameCount frames to pass (since lastFrame, if not 0) before continuing
	void waitFrame(unsigned int frameCount=1, unsigned int lastFrame=0);

	// Returns true if both input structures have been initalized.
	bool InputInitalized();
	// Blocking
	// Wait for input to be initialized before continuing
	void waitInputInit();

	/*
	// Static
	*/

	void nop(unsigned int baseAddress, unsigned int size);
	void keepActive();
	void nop2PSpecials();
	void nopP2Input(bool doNop=true);
	void swapSpawn(bool swapstart);
	void swapCharsel(bool swapcharsel);
	void swapInput(bool doNop=true);

	void changeGameState(uchar stateNum, MemStruct* structure);
};