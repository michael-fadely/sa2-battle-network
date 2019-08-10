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

static constexpr ushort analog_threshold = 16;
static constexpr uint   analog_frames    = 8;

static uint analog_timer = 0;
static uint angle_timer  = 0;
static bool send_angle   = false;

static PDS_PERIPHERAL net_input[ControllerPointers_Length];
static PolarCoord net_analog[ControllerPointers_Length];

using namespace nethax;
using namespace globals;

sws::Packet& operator<<(sws::Packet& lhs, const AnalogThing& rhs)
{
	return lhs << *(const PolarCoord*)(&rhs);
}

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
			packet << pad->x1 << pad->y1;
			net_pad.x1 = pad->x1;
			net_pad.y1 = pad->y1;
			broker->append(MessageID::I_Analog, protocol, &packet);
			break;
		}

		case MessageID::I_AnalogAngle:
		{
			sws::Packet packet;
			packet << AnalogThings[pnum];
			net_analog[pnum] = *reinterpret_cast<PolarCoord*>(&AnalogThings[pnum]);
			broker->append(MessageID::I_AnalogAngle, protocol, &packet);
			break;
		}

		case MessageID::I_Buttons:
		{
			sws::Packet packet;
			packet << pad->on;
			net_pad.on = pad->on;
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
			PDS_PERIPHERAL* pad = ControllerPointers[i];
			PDS_PERIPHERAL* net_pad = &net_input[i];

#pragma region Send
			if (static_cast<pnum_t>(i) == pnum)
			{
				bool sent_buttons = false;

				if (pad->press || pad->release)
				{
					send(MessageID::I_Buttons, Protocol::tcp, static_cast<pnum_t>(i));
					sent_buttons = true;
				}

				// TODO: Make less spammy
				if (pad->x1 != net_pad->x1 || pad->y1 != net_pad->y1)
				{
					++analog_timer %= (analog_frames / FrameIncrement);

					if ((abs(net_pad->x1 - pad->x1) >= analog_threshold || abs(net_pad->y1 - pad->y1) >= analog_threshold)
						|| !analog_timer)
					{
						analog_timer = 0;
						send(MessageID::I_Analog, Protocol::udp, static_cast<pnum_t>(i));
						send_angle = true;
					}
					else if (!pad->x1 && !pad->y1)
					{
						send(MessageID::I_Analog, Protocol::tcp, static_cast<pnum_t>(i));
						send_angle = true;
					}
				}

				send_angle = send_angle || sent_buttons;
				broker->finalize();
				continue;
			}
#pragma endregion

			pad->x1 = net_pad->x1;
			pad->y1 = net_pad->y1;

			pad->x2 = net_pad->x2;
			pad->y2 = net_pad->y2;

			pad->on    = net_pad->on;
			pad->off = ~pad->on;

			// Here we're using netPad's "old" since it can in some cases be overwritten
			// by the input update with irrelevant data, thus invalidating Released and Pressed.
			const uint mask      = (pad->on ^ net_pad->old);
			pad->release = net_pad->old & mask;
			pad->press  = pad->on & mask;

			// Setting pad->old might not be necessary, but better safe than sorry.
			net_pad->old = pad->old = pad->on;

			// HACK: Fixes camera rotation in non-emerald hunting modes.
			pad->l = (pad->on & Buttons_L) ? UCHAR_MAX : 0;
			pad->r = (pad->on & Buttons_R) ? UCHAR_MAX : 0;
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
			const PolarCoord& current = *reinterpret_cast<PolarCoord*>(&AnalogThings[i]);
			const PolarCoord& net = net_analog[i];

			if (static_cast<pnum_t>(i) == pnum)
			{
				RumblePort_A[i] = 0;

				bool dir_delta = abs(net.angle - current.angle) >= 2048;
				bool mag_delta = fabs(current.distance - net.distance) >= 0.0625f;
				send_angle = send_angle || (++angle_timer %= (analog_frames / FrameIncrement)) == 0;

				if (dir_delta || mag_delta || (send_angle && (current.angle != net.angle || fabs(current.distance - net.distance) >= FLT_EPSILON)))
				{
#ifdef _DEBUG
					PrintDebug("[%04d]\t\tDIR: %d MAG: %d TIMER: %d", FrameCount, dir_delta, mag_delta, (!dir_delta && !mag_delta));
#endif
					send(MessageID::I_AnalogAngle, Protocol::udp, static_cast<pnum_t>(i));
					send_angle = false;
				}

				continue;
			}

			RumblePort_A[i] = -1;
			auto& pad = net_input[i];
			if (pad.x1 != 0 || pad.y1 != 0)
			{
				AnalogThings[i] = *reinterpret_cast<const AnalogThing*>(&net);
			}
		}
	}
}

static bool MessageHandler(MessageID id, int pnum, sws::Packet& packet)
{
	if (CurrentMenu == Menu::battle || (TwoPlayerMode > 0 && GameState > GameState::Inactive))
	{
		switch (id)
		{
			default:
				return false;

			case MessageID::I_Buttons:
				packet >> net_input[pnum].on;
				break;

			case MessageID::I_Analog:
				packet >> net_input[pnum].x1 >> net_input[pnum].y1;
				break;

			case MessageID::I_AnalogAngle:
				packet >> net_analog[pnum];
				AnalogThings[pnum] = *reinterpret_cast<AnalogThing*>(&net_analog[pnum]);
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

void swap_input(pnum_t from, pnum_t to)
{
	swap(ControllerPointers, from, to);
	swap(net_input, from, to);
	swap(net_analog, from, to);
}
