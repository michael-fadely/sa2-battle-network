#include <cmath>			// for abs

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"		// for MSG
#include "Globals.h"		// for Globals :specialed:
#include "AddressList.h"	// for FrameCount, FrameIncrement

#include "OnInput.h"

#pragma region Initialization

void* OnInput_ptr = (void*)0x0077E897;
void __cdecl OnInput()
{
	InputHandler();
}

void InitOnInput()
{
	char* buffer = new char[9];
	memset(buffer, 0xC3, 9);
	WriteData(OnInput_ptr, buffer, 9);
	WriteCall(OnInput_ptr, OnInput);
	delete[] buffer;
}

#pragma endregion

static const ushort analogThreshold = 16;
static const ushort analogMax = 220;
static const uint analogFrames = 8;
static const uint lastFrame = 0;

void InputHandler()
{
	using namespace sa2bn;

	if (!Globals::isConnected())
		return;

	PacketBroker* broker = Globals::Broker;
	ControllerData* pad = &ControllersRaw[0];
	ControllerData* netPad = &broker->recvInput;
	ControllerData* lastPad = &broker->sendInput;

#pragma region Send
	if (pad->PressedButtons || pad->ReleasedButtons)
		broker->Request(MSG_I_BUTTONS, true);

	if (pad->LeftStickX != lastPad->LeftStickX || pad->LeftStickY != lastPad->LeftStickY)
	{
		if ((abs(lastPad->LeftStickX - pad->LeftStickX) >= analogThreshold || abs(lastPad->LeftStickY - pad->LeftStickY) >= analogThreshold)
			|| (FrameCount - lastFrame) > (analogFrames / FrameIncrement))
		{
			broker->Request(MSG_I_ANALOG, false);
		}
		else if (!pad->LeftStickX && !pad->LeftStickY)
		{
			broker->Request(MSG_I_ANALOG, true);
		}
	}

	broker->Finalize();
#pragma endregion

	pad = &ControllersRaw[1];

	pad->LeftStickX = netPad->LeftStickX;
	pad->LeftStickY = netPad->LeftStickY;

	pad->RightStickX = netPad->RightStickX;
	pad->RightStickY = netPad->RightStickY;

	pad->HeldButtons = netPad->HeldButtons;
	pad->NotHeldButtons = ~pad->HeldButtons;

	pad->ReleasedButtons = pad->Old & (pad->HeldButtons ^ pad->Old);

	pad->PressedButtons = pad->HeldButtons & (pad->HeldButtons ^ pad->Old);

	pad->Old = pad->HeldButtons;

	pad->LTriggerPressure = (pad->HeldButtons & Buttons_L) ? UCHAR_MAX : 0;
	pad->RTriggerPressure = (pad->HeldButtons & Buttons_R) ? UCHAR_MAX : 0;
}