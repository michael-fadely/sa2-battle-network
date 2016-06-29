#include "stdafx.h"
#include <Trampoline.h>
#include "Globals.h"

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
	using namespace Globals;
	using namespace random;

	current_seed = seed;

	if (!isConnected())
	{
		srand_original(seed);
		return;
	}

	if (Networking->isServer())
	{
		srand_original(seed);

		if (Broker->WaitForPlayers(MessageID::S_Seed))
		{
			PrintDebug("<< Sending seed: 0x%08X", current_seed);

			PacketEx packet(Protocol::TCP);

			packet.AddType(MessageID::S_Seed);
			packet << current_seed;
			Broker->AddTypeSent(MessageID::S_Seed, packet.GetTypeSize(), packet.Protocol);
			packet.Finalize();

			Broker->AddReady(MessageID::S_Seed, packet);
			Broker->Send(packet);
		}
	}
	else
	{
		if (!Broker->SendReadyAndWait(MessageID::S_Seed))
			srand_original(seed);
	}
}

static bool MessageHandler(MessageID type, PlayerNumber pnum, sf::Packet& packet)
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

void random::InitRandom()
{
	Globals::Broker->RegisterMessageHandler(MessageID::S_Seed, MessageHandler);
	srand_trampoline = new Trampoline(0x007A89C6, 0x007A89CB, srand_hook);
}

void random::DeinitRandom()
{
	delete srand_trampoline;
}
