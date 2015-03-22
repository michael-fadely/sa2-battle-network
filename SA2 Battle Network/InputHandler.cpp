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

void InputHandler()
{
	// Placeholder
	ControllerData local = {};
	ControllerData network = {};

	// Network input
	local.LeftStickX = network.LeftStickX;
	local.LeftStickY = network.LeftStickY;

	local.RightStickX = network.RightStickX;
	local.RightStickY = network.RightStickY;

	local.LTriggerPressure = network.LTriggerPressure;
	local.RTriggerPressure = network.RTriggerPressure;

	local.HeldButtons = network.HeldButtons;
	local.NotHeldButtons = ~local.HeldButtons;
	
	// Simulated button presses
	local.ReleasedButtons = local.HeldButtons ^ local.Old;
	local.PressedButtons = local.HeldButtons & ~local.Old;

	local.Old = local.HeldButtons;
}