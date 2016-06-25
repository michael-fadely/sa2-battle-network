#include "stdafx.h"

#include "OnInput.h"

#include <cmath>			// for abs

#include <SA2ModLoader.h>	// for everything
#include "Networking.h"
#include "AdventurePacketOverloads.h"
#include "Globals.h"		// for Globals :specialed:
#include "AddressList.h"	// for FrameCount, FrameIncrement
#include "nop.h"
#include "CommonEnums.h"

static const ushort analogThreshold = 16;
static const uint analogFrames = 8;

static uint analogTimer = 0;
static uint angleTimer = 0;
static bool sendAngle = false;

static ControllerData net_input[ControllerPointers_Length];
static PolarCoord net_analog[ControllerPointers_Length];

using namespace nethax;
using namespace Globals;

static void send(MessageID type, nethax::Protocol protocol, PlayerNumber pnum)
{
	auto& pad = ControllerPointers[pnum];
	auto& netPad = net_input[pnum];

	switch (type)
	{
		default:
			break;

		case MessageID::I_Analog:
		{
			sf::Packet packet;
			packet << pad->LeftStickX << pad->LeftStickY;
			netPad.LeftStickX = pad->LeftStickX;
			netPad.LeftStickY = pad->LeftStickY;
			Broker->Append(MessageID::I_Analog, protocol, &packet);
			break;
		}

		case MessageID::I_AnalogAngle:
		{
			sf::Packet packet;
			packet << AnalogThings[pnum];
			net_analog[pnum] = AnalogThings[pnum];
			Broker->Append(MessageID::I_AnalogAngle, protocol, &packet);
			break;
		}

		case MessageID::I_Buttons:
		{
			sf::Packet packet;
			packet << pad->HeldButtons;
			netPad.HeldButtons = pad->HeldButtons;
			Broker->Append(MessageID::I_Buttons, protocol, &packet);
			break;
		}
	}
}

extern "C"
{
	void __declspec(dllexport) OnInput()
	{
		if (!isConnected())
			return;

		auto pnum = Broker->GetPlayerNumber();


		for (auto i = 0; i < 4; i++)
		{
			ControllerData* pad = ControllerPointers[i];
			ControllerData* netPad = &net_input[i];

			if (i == pnum)
			{
#pragma region Send
				bool sentButtons = false;
				if (pad->PressedButtons || pad->ReleasedButtons)
				{
					send(MessageID::I_Buttons, Protocol::TCP, i);
					sentButtons = true;
				}

				// TODO: Make less spammy
				if (pad->LeftStickX != netPad->LeftStickX || pad->LeftStickY != netPad->LeftStickY)
				{
					++analogTimer %= (analogFrames / FrameIncrement);
					if ((abs(netPad->LeftStickX - pad->LeftStickX) >= analogThreshold || abs(netPad->LeftStickY - pad->LeftStickY) >= analogThreshold)
						|| !analogTimer)
					{
						analogTimer = 0;
						send(MessageID::I_Analog, Protocol::UDP, i);
						sendAngle = true;
					}
					else if (!pad->LeftStickX && !pad->LeftStickY)
					{
						send(MessageID::I_Analog, Protocol::TCP, i);
						sendAngle = true;
					}
				}

				sendAngle = sendAngle || sentButtons;
				Broker->Finalize();
			}
#pragma endregion
			else
			{
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
		}
	}

	void __declspec(dllexport) OnControl()
	{
		if (!isConnected())
			return;

		auto pnum = Broker->GetPlayerNumber();

		for (auto i = 0; i < 4; i++)
		{
			const PolarCoord& current = AnalogThings[i];
			const PolarCoord& net = net_analog[i];

			if (i == pnum)
			{
				RumblePort_A[i] = 0;

				bool dir_delta = abs(net.angle - current.angle) >= 2048;
				bool mag_delta = fabs(current.distance - net.distance) >= 0.0625f;
				sendAngle = sendAngle || (++angleTimer %= (analogFrames / FrameIncrement)) == 0;

				if (dir_delta || mag_delta || sendAngle && (current.angle != net.angle || fabs(current.distance - net.distance) >= FLT_EPSILON))
				{
#ifdef _DEBUG
					PrintDebug("[%04d]\t\tDIR: %d MAG: %d TIMER: %d", FrameCount, dir_delta, mag_delta, (!dir_delta && !mag_delta));
#endif
					send(MessageID::I_AnalogAngle, Protocol::UDP, i);
					sendAngle = false;
				}
			}
			else
			{
				RumblePort_A[i] = -1;
				auto& pad = net_input[i];
				if (pad.LeftStickX != 0 || pad.LeftStickY != 0)
					AnalogThings[i] = net;
			}
		}
	}
}

static bool MessageHandler(MessageID id, int pnum, sf::Packet& packet)
{
	if (CurrentMenu[0] == Menu::BATTLE || TwoPlayerMode > 0 && GameState > GameState::Inactive)
	{
		switch (id)
		{
			default:
				return false;

			case MessageID::I_Buttons:
				packet >> net_input[pnum].HeldButtons;
				break;

				case MessageID::I_Analog:
				packet >> net_input[pnum].LeftStickX >> net_input[pnum].LeftStickY;
				break;

				case MessageID::I_AnalogAngle:
				packet >> net_analog[pnum];
				AnalogThings[pnum] = net_analog[pnum];
				break;
		}

		return true;
	}

	return false;
}

void InitOnInput()
{
	// Control
	nop::apply(0x00441BCA, 7);
	nop::apply(0x00441BFC, 11);
	nop::apply(0x00441C1C, 10);
	// Control_b
	nop::apply(0x00441D7A, 7);
	nop::apply(0x00441DAC, 11);
	nop::apply(0x00441DCC, 10);

	Broker->RegisterMessageHandler(MessageID::I_Buttons, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::I_Analog, MessageHandler);
	Broker->RegisterMessageHandler(MessageID::I_AnalogAngle, MessageHandler);
	
	for (auto i = 0; i < 4; i++)
	{
		net_input[i] = {};
		net_analog[i] = {};
	}
}

void DeinitOnInput()
{
	// Control
	nop::restore(0x00441BCA);
	nop::restore(0x00441BFC);
	nop::restore(0x00441C1C);
	// Control_b
	nop::restore(0x00441D7A);
	nop::restore(0x00441DAC);
	nop::restore(0x00441DCC);
}

template<typename T> void swap(T& a, int from, int to)
{
	auto last = a[to];
	a[to] = a[from];
	a[from] = last;
}

void SwapInput(PlayerNumber from, PlayerNumber to)
{
	swap(ControllerPointers, from, to);
	swap(net_input, from, to);
	swap(net_analog, from, to);
}
