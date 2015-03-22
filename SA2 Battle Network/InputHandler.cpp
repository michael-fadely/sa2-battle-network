#include <SA2ModLoader.h>
#include <LazyTypedefs.h>
#include "Globals.h"

#include "InputHandler.h"

void __cdecl OnInput()
{
	if (true)
		InputHandler();
}

void* OnInput_Ptr = (void*)0x0077E897;

void InitInputHandler()
{
	char* returns = new char[9];
	memset(returns, 0xC3, 9);
	WriteData(OnInput_Ptr, returns, 9);
	WriteCall(OnInput_Ptr, OnInput);
}

// Placeholders for not-yet-implemented features.
const uint MyPlayerID = 0;
const uint PlayerCount = 2;

// TODO: Send input from here.
void InputHandler()
{
	using namespace sa2bn;

	if (!Globals::isConnected())
		return;

	Globals::Memory->inputLock.lock();
	ControllerData* network = &Globals::Memory->recvInput;
	ControllerData* pad = &ControllersRaw[1];

	pad->LeftStickX = network->LeftStickX;
	pad->LeftStickY = network->LeftStickY;

	pad->RightStickX = network->RightStickX;
	pad->RightStickY = network->RightStickY;

	pad->LTriggerPressure = network->LTriggerPressure;
	pad->RTriggerPressure = network->RTriggerPressure;

	pad->HeldButtons = network->HeldButtons;
	pad->NotHeldButtons = ~pad->HeldButtons;

	pad->ReleasedButtons = pad->Old & (pad->HeldButtons ^ pad->Old);

	pad->PressedButtons = pad->HeldButtons & (pad->HeldButtons ^ pad->Old);

	pad->Old = pad->HeldButtons;

	Globals::Memory->inputLock.unlock();
}