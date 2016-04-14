#include "stdafx.h"
#include <Trampoline.h>
#include "Globals.h"

#include "Random.h"

using namespace nethax;

//FunctionPointer(void, _srand, (int seed), 0x007A89C6);
unsigned int random::current_seed = 0;

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
			PacketEx packet(Protocol::TCP);
			Broker->Request(MessageID::S_Seed, packet, true);
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

Trampoline random::srand_trampoline(0x007A89C6, 0x007A89CB, (DetourFunction)srand_hook);
