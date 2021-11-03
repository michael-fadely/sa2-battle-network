#include "stdafx.h"
#include <Trampoline.h>
#include "globals.h"

#include "Random.h"
#include "FunctionPointers.h"

using namespace nethax;

unsigned int random::current_seed = 0;
static Trampoline* srand_trampoline = nullptr;

void random::srand_original(unsigned int seed)
{
	_FunctionPointer(void, target, (unsigned int), srand_trampoline->Target());
	target(seed);
}

void __cdecl random::srand_hook(unsigned int seed)
{
	using namespace globals;
	using namespace random;

	current_seed = seed;

	if (!is_connected())
	{
		srand_original(seed);
		return;
	}

	if (broker->is_server())
	{
		srand_original(seed);

		if (broker->wait_for_players(MessageID::S_Seed))
		{
			PrintDebug("<< Sending seed: 0x%08X", current_seed);

			PacketEx packet(Protocol::tcp);

			packet.add_type(MessageID::S_Seed);
			packet << current_seed;
			broker->add_type_sent(MessageID::S_Seed, packet.get_type_size(), packet.protocol);
			packet.finalize();

			broker->add_ready(MessageID::S_Seed, packet);
			broker->send(packet);
		}
	}
	else if (!broker->send_ready_and_wait(MessageID::S_Seed))
	{
		srand_original(seed);
	}
}

static bool message_reader(MessageID type, pnum_t pnum, sws::Packet& packet)
{
	switch (type)
	{
		default:
			return false;

		case MessageID::S_Seed:
			packet >> random::current_seed;
			PrintDebug(">> Received seed: 0x%08X", random::current_seed);
			random::srand_original(random::current_seed);
			return true;
	}
}

void random::init()
{
	globals::broker->register_reader(MessageID::S_Seed, message_reader);
	srand_trampoline = new Trampoline(0x007A89C6, 0x007A89CB, srand_hook);
}

void random::deinit()
{
	delete srand_trampoline;
}
