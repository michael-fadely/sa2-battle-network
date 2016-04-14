#include "stdafx.h"

#include <cmath>			// for abs

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"
#include "Globals.h"		// for Globals :specialed:
#include "AddressList.h"	// for FrameCount, FrameIncrement

static const ushort analogThreshold = 16;
static const uint analogFrames = 8;

static uint analogTimer = 0;
static uint angleTimer = 0;
static bool sendAngle = false;

using namespace nethax;
using namespace Globals;

extern "C"
{
	void __declspec(dllexport) OnInput()
	{
		if (!isConnected())
			return;

		ControllerData* pad = ControllerPointers[0];
		ControllerData* netPad = &Broker->recvInput;
		ControllerData* lastPad = &Broker->sendInput;

#pragma region Send
		bool sentButtons = false;
		if (pad->PressedButtons || pad->ReleasedButtons)
		{
			Broker->Request(MessageID::I_Buttons, Protocol::TCP);
			sentButtons = true;
		}

		// TODO: Make less spammy
		if (pad->LeftStickX != lastPad->LeftStickX || pad->LeftStickY != lastPad->LeftStickY)
		{
			++analogTimer %= (analogFrames / FrameIncrement);
			if ((abs(lastPad->LeftStickX - pad->LeftStickX) >= analogThreshold || abs(lastPad->LeftStickY - pad->LeftStickY) >= analogThreshold)
				|| !analogTimer)
			{
				analogTimer = 0;
				Broker->Request(MessageID::I_Analog, Protocol::UDP);
				sendAngle = true;
			}
			else if (!pad->LeftStickX && !pad->LeftStickY)
			{
				Broker->Request(MessageID::I_Analog, Protocol::TCP);
				sendAngle = true;
			}
		}

		sendAngle = sendAngle || sentButtons;
		Broker->Finalize();
#pragma endregion

		pad = ControllerPointers[1];

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

	void __declspec(dllexport) OnControl()
	{
		if (!isConnected())
			return;

		const PolarCoord& current = AnalogThings[0];
		const PolarCoord& net = Broker->sendAnalog;

		bool dir_delta = abs(net.angle - current.angle) >= 2048;
		bool mag_delta = fabs(current.distance - net.distance) >= 0.0625f;
		sendAngle = sendAngle || (++angleTimer %= (analogFrames / FrameIncrement)) == 0;

		if (dir_delta || mag_delta || sendAngle && (current.angle != net.angle || fabs(current.distance - net.distance) >= FLT_EPSILON))
		{
#ifdef _DEBUG
			PrintDebug("[%04d]\t\tDIR: %d MAG: %d TIMER: %d", FrameCount, dir_delta, mag_delta, (!dir_delta && !mag_delta));
#endif
			Broker->Request(MessageID::I_AnalogAngle, Protocol::UDP);
			sendAngle = false;
		}

		if (Broker->recvInput.LeftStickX != 0 || Broker->recvInput.LeftStickY != 0)
			AnalogThings[1] = Broker->recvAnalog;
	}
}
