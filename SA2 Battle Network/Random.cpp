#include "stdafx.h"
#include <Trampoline.h>
#include "Globals.h"

#include "Random.h"

using namespace nethax;

unsigned int current_seed = 0;

//FunctionPointer(void, _srand, (int seed), 0x007A89C6);

void __cdecl hook_srand(unsigned int seed)
{
	using namespace Globals;

	current_seed = seed;

	if (!isConnected())
	{
		srand_Original(seed);
		return;
	}

	if (Networking->isServer())
	{
		srand_Original(seed);

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
			return;
		}
		else
		{
			srand_Original(seed);
		}
	}
}

Trampoline srand_t(0x007A89C6, 0x007A89CB, (DetourFunction)hook_srand);
