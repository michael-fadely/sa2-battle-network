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

		Broker->Request(MessageID::S_Seed, true, true);
		Broker->Finalize();
		Broker->SendReady(MessageID::S_Seed);
		Broker->WaitForPlayers(MessageID::S_Seed);
	}
	else
	{
		if (Broker->WaitForPlayers(MessageID::S_Seed))
		{
			Broker->SendReady(MessageID::S_Seed);
		}
		else
		{
			srand_original(seed);
		}
	}
}

Trampoline random::srand_t(0x007A89C6, 0x007A89CB, (DetourFunction)srand_hook);
