#include "stdafx.h"

#include "OnInput.h"

#include <cmath>

#include <SA2ModLoader.h>
#include "Networking.h"
#include "AdventurePacketOverloads.h"
#include "globals.h"
#include "AddressList.h"
#include "nop.h"
#include "CommonEnums.h"

static const ushort ANALOG_THRESHOLD = 16;
static const uint   ANALOG_FRAMES    = 8;

static uint analog_timer = 0;
static uint angle_timer  = 0;
static bool send_angle   = false;

static ControllerData net_input[ControllerPointers_Length];
static PolarCoord net_analog[ControllerPointers_Length];

using namespace nethax;
using namespace globals;

static void send(MessageID type, Protocol protocol, pnum_t pnum)
{
	auto& pad = ControllerPointers[pnum];
	auto& net_pad = net_input[pnum];

	switch (type)
	{
		default:
			break;

		case MessageID::I_Analog:
		{
			sws::Packet packet;
			packet << pad->LeftStickX << pad->LeftStickY;
			net_pad.LeftStickX = pad->LeftStickX;
			net_pad.LeftStickY = pad->LeftStickY;
			broker->append(MessageID::I_Analog, protocol, &packet);
			break;
		}

		case MessageID::I_AnalogAngle:
		{
			sws::Packet packet;
			packet << AnalogThings[pnum];
			net_analog[pnum] = AnalogThings[pnum];
			broker->append(MessageID::I_AnalogAngle, protocol, &packet);
			break;
		}

		case MessageID::I_Buttons:
		{
			sws::Packet packet;
			packet << pad->HeldButtons;
			net_pad.HeldButtons = pad->HeldButtons;
			broker->append(MessageID::I_Buttons, protocol, &packet);
			break;
		}
	}
}

extern "C"
{
	void __declspec(dllexport) OnInput()
	{
		if (!is_connected())
		{
			return;
		}

		auto pnum = broker->get_player_number();

		for (size_t i = 0; i < 4; i++)
		{
			ControllerData* pad = ControllerPointers[i];
			ControllerData* net_pad = &net_input[i];

#pragma region Send
			if (i == pnum)
			{
				bool sent_buttons = false;

				if (pad->PressedButtons || pad->ReleasedButtons)
				{
					send(MessageID::I_Buttons, Protocol::tcp, i);
					sent_buttons = true;
				}

				// TODO: Make less spammy
				if (pad->LeftStickX != net_pad->LeftStickX || pad->LeftStickY != net_pad->LeftStickY)
				{
					++analog_timer %= (ANALOG_FRAMES / FrameIncrement);

					if ((abs(net_pad->LeftStickX - pad->LeftStickX) >= ANALOG_THRESHOLD || abs(net_pad->LeftStickY - pad->LeftStickY) >= ANALOG_THRESHOLD)
						|| !analog_timer)
					{
						analog_timer = 0;
						send(MessageID::I_Analog, Protocol::udp, i);
						send_angle = true;
					}
					else if (!pad->LeftStickX && !pad->LeftStickY)
					{
						send(MessageID::I_Analog, Protocol::tcp, i);
						send_angle = true;
					}
				}

				send_angle = send_angle || sent_buttons;
				broker->finalize();
				continue;
			}
#pragma endregion

			pad->LeftStickX = net_pad->LeftStickX;
			pad->LeftStickY = net_pad->LeftStickY;

			pad->RightStickX = net_pad->RightStickX;
			pad->RightStickY = net_pad->RightStickY;

			pad->HeldButtons    = net_pad->HeldButtons;
			pad->NotHeldButtons = ~pad->HeldButtons;

			// Here we're using netPad's "Old" since it can in some cases be overwritten
			// by the input update with irrelevant data, thus invalidating Released and Pressed.
			const uint mask      = (pad->HeldButtons ^ net_pad->Old);
			pad->ReleasedButtons = net_pad->Old & mask;
			pad->PressedButtons  = pad->HeldButtons & mask;

			// Setting pad->Old might not be necessary, but better safe than sorry.
			net_pad->Old = pad->Old = pad->HeldButtons;

			// HACK: Fixes camera rotation in non-emerald hunting modes.
			pad->LTriggerPressure = (pad->HeldButtons & Buttons_L) ? UCHAR_MAX : 0;
			pad->RTriggerPressure = (pad->HeldButtons & Buttons_R) ? UCHAR_MAX : 0;
		}
	}

	void __declspec(dllexport) OnControl()
	{
		if (!is_connected())
		{
			return;
		}

		auto pnum = broker->get_player_number();

		for (size_t i = 0; i < 4; i++)
		{
			const PolarCoord& current = AnalogThings[i];
			const PolarCoord& net = net_analog[i];

			if (static_cast<pnum_t>(i) == pnum)
			{
				RumblePort_A[i] = 0;

				bool dir_delta = abs(net.angle - current.angle) >= 2048;
				bool mag_delta = fabs(current.distance - net.distance) >= 0.0625f;
				send_angle = send_angle || (++angle_timer %= (ANALOG_FRAMES / FrameIncrement)) == 0;

				if (dir_delta || mag_delta || (send_angle && (current.angle != net.angle || fabs(current.distance - net.distance) >= FLT_EPSILON)))
				{
#ifdef _DEBUG
					PrintDebug("[%04d]\t\tDIR: %d MAG: %d TIMER: %d", FrameCount, dir_delta, mag_delta, (!dir_delta && !mag_delta));
#endif
					send(MessageID::I_AnalogAngle, Protocol::udp, i);
					send_angle = false;
				}

				continue;
			}

			RumblePort_A[i] = -1;
			auto& pad = net_input[i];
			if (pad.LeftStickX != 0 || pad.LeftStickY != 0)
			{
				AnalogThings[i] = net;
			}
		}
	}
}

static bool MessageHandler(MessageID id, int pnum, sws::Packet& packet)
{
	if (CurrentMenu[0] == Menu::battle || (TwoPlayerMode > 0 && GameState > GameState::Inactive))
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

	broker->register_message_handler(MessageID::I_Buttons, MessageHandler);
	broker->register_message_handler(MessageID::I_Analog, MessageHandler);
	broker->register_message_handler(MessageID::I_AnalogAngle, MessageHandler);
	
	for (size_t i = 0; i < 4; i++)
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

void SwapInput(pnum_t from, pnum_t to)
{
	swap(ControllerPointers, from, to);
	swap(net_input, from, to);
	swap(net_analog, from, to);
}
