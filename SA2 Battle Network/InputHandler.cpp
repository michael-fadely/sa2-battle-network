#include <SA2ModLoader.h>
#include <LazyTypedefs.h>
#include "Globals.h"

#include "InputHandler.h"

void* OnInput_Ptr = (void*)0x0077E897;
void __cdecl OnInput()
{
	InputHandler();
}

void InitInputHandler()
{
	char* returns = new char[9];
	memset(returns, 0xC3, 9);
	WriteData(OnInput_Ptr, returns, 9);
	WriteCall(OnInput_Ptr, OnInput);
	delete[] returns;
}

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

	pad->HeldButtons = network->HeldButtons;
	pad->NotHeldButtons = ~pad->HeldButtons;

	pad->ReleasedButtons = pad->Old & (pad->HeldButtons ^ pad->Old);

	pad->PressedButtons = pad->HeldButtons & (pad->HeldButtons ^ pad->Old);

	pad->Old = pad->HeldButtons;

	pad->LTriggerPressure = (pad->HeldButtons & Buttons_L) ? UCHAR_MAX : 0;
	pad->RTriggerPressure = (pad->HeldButtons & Buttons_R) ? UCHAR_MAX : 0;

	Globals::Memory->inputLock.unlock();
}