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
	// TODO: Figure out why this breaks if not finalized here
	if (pad->PressedButtons || pad->ReleasedButtons)
		broker->Request(MSG_I_BUTTONS, true);

	// TODO: Figure out why this is so freaking slow (aside from the obvious huge amount of packets)
	if (pad->LeftStickX != lastPad->LeftStickX || pad->LeftStickY != lastPad->LeftStickY)
	{
		if (!pad->LeftStickX && !pad->LeftStickY)
			broker->Request(MSG_I_ANALOG, true);
		else if (FrameCount % (2 - (FrameIncrement - 1)))
			broker->Request(MSG_I_ANALOG, false);
	}

	broker->Finalize();
#pragma endregion

	broker->inputLock.lock();

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

	broker->inputLock.unlock();
}