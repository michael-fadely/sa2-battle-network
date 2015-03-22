#include <SA2ModLoader.h>
#include <LazyTypedefs.h>

#include "Globals.h"

void InputHandler();

extern "C"
{
	void __declspec(dllexport) __cdecl OnInput()
	{
		if (true)
			InputHandler();
	}
}

// Placeholders for not-yet-implemented features.
const uint MyPlayerID = 0;
const uint PlayerCount = 2;

void InputHandler()
{
	using namespace sa2bn;

	if (!Globals::isConnected())
		return;

	Globals::Memory->inputLock.lock();
	ControllerData* network = &Globals::Memory->recvInput;
	
	for (uint i = 0; i < PlayerCount; i++)
	{
		// TODO: Send input from here.
		if (i == MyPlayerID)
			continue;
	
		ControllerData* local = &ControllersRaw[i];

		// Network input
		local->LeftStickX = network->LeftStickX;
		local->LeftStickY = network->LeftStickY;

		local->RightStickX = network->RightStickX;
		local->RightStickY = network->RightStickY;

		local->LTriggerPressure = network->LTriggerPressure;
		local->RTriggerPressure = network->RTriggerPressure;

		local->HeldButtons = network->HeldButtons;
		local->NotHeldButtons = ~local->HeldButtons;

		// Simulated button presses
		local->ReleasedButtons = local->HeldButtons ^ local->Old;
		local->PressedButtons = local->HeldButtons & ~local->Old;

		local->Old = local->HeldButtons;
	}

	Globals::Memory->inputLock.unlock();
}