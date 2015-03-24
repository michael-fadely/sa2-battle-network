#include <SA2ModLoader.h>
#include "Networking.h"
#include "Globals.h"

#include "OnInput.h"

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

// TODO: Send input from here.
void InputHandler()
{
	using namespace sa2bn;

	if (!Globals::isConnected())
		return;

	MemoryHandler* memory = Globals::Memory;
	ControllerData* pad = &ControllersRaw[0];
	ControllerData* netPad = &memory->recvInput;
	ControllerData* lastPad = &memory->sendInput;

#pragma region Send
	{
		PacketEx safe(true), fast(false);

		if (pad->PressedButtons || pad->ReleasedButtons)
			memory->RequestPacket(MSG_I_BUTTONS, safe);

		if (pad->LeftStickX != lastPad->LeftStickX || pad->LeftStickY != lastPad->LeftStickY)
			memory->RequestPacket(MSG_I_ANALOG, (!pad->LeftStickX && !pad->LeftStickY) ? safe : fast);

		Globals::Networking->Send(safe);
		Globals::Networking->Send(fast);
	}
#pragma endregion

	memory->inputLock.lock();

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

	memory->inputLock.unlock();
}