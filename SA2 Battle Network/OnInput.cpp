#include "stdafx.h"

#include <cmath>			// for abs

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MSG
#include "Globals.h"		// for Globals :specialed:
#include "AddressList.h"	// for FrameCount, FrameIncrement

static const ushort analogThreshold = 16;
static const ushort analogMax = 220;
static const uint analogFrames = 8;
static uint lastFrame = 0;

void InputHandler()
{
	using namespace nethax;
	using namespace Globals;

	if (!isInitialized() || !isConnected())
		return;

	ControllerData* pad = &ControllersRaw[0];
	ControllerData* netPad = &Broker->recvInput;
	ControllerData* lastPad = &Broker->sendInput;

#pragma region Send
	if (pad->PressedButtons || pad->ReleasedButtons)
		Broker->Request(Message::I_Buttons, true);

	// TODO: Make less spammy
	if (pad->LeftStickX != lastPad->LeftStickX || pad->LeftStickY != lastPad->LeftStickY)
	{
		if (
			(abs(lastPad->LeftStickX - pad->LeftStickX) >= analogThreshold || abs(lastPad->LeftStickY - pad->LeftStickY) >= analogThreshold)
			|| (FrameCount - lastFrame) > (analogFrames / FrameIncrement)
			)
		{
			lastFrame = FrameCount;
			Broker->Request(Message::I_Analog, false);
		}
		else if (!pad->LeftStickX && !pad->LeftStickY)
		{
			Broker->Request(Message::I_Analog, true);
		}
	}

	Broker->Finalize();
#pragma endregion

	pad = &ControllersRaw[1];

	pad->LeftStickX = netPad->LeftStickX;
	pad->LeftStickY = netPad->LeftStickY;

	pad->RightStickX = netPad->RightStickX;
	pad->RightStickY = netPad->RightStickY;

	pad->HeldButtons = netPad->HeldButtons;
	pad->NotHeldButtons = ~pad->HeldButtons;

	// Here we're using netPad's "Old" since it can in some cases be overwritten
	// by the input update with irrelevant data, thus invalidating Released and Pressed.
	uint mask = (pad->HeldButtons ^ netPad->Old);
	pad->ReleasedButtons = netPad->Old & mask;
	pad->PressedButtons = pad->HeldButtons & mask;

	// Setting pad->Old might not be necessary, but better safe than sorry.
	netPad->Old = pad->Old = pad->HeldButtons;

	// HACK: Fixes camera rotation in non-emerald hunting modes.
	pad->LTriggerPressure = (pad->HeldButtons & Buttons_L) ? UCHAR_MAX : 0;
	pad->RTriggerPressure = (pad->HeldButtons & Buttons_R) ? UCHAR_MAX : 0;
}

extern "C" void __declspec(dllexport) OnInput()
{
	InputHandler();
}
