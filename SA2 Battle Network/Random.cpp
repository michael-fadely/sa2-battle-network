#include "stdafx.h"
#include <Trampoline.h>
#include "Globals.h"

#include "Random.h"
#include "FunctionPointers.h"

using namespace nethax;

//FunctionPointer(void, _srand, (int seed), 0x007A89C6);
unsigned int random::current_seed = 0;

Trampoline* srand_trampoline;

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
		Broker->SendReady(MessageID::S_Seed);

		if (!Broker->WaitForPlayers(MessageID::S_Seed))
			srand_original(seed);
	}
}

void random::InitRandom()
{
	srand_trampoline = new Trampoline(0x007A89C6, 0x007A89CB, srand_hook);
}

void random::DeinitRandom()
{
	delete srand_trampoline;
}
