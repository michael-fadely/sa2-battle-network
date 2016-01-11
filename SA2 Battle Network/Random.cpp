#include "stdafx.h"
#include "Trampoline.h"
#include "Globals.h"

#include "Random.h"

using namespace nethax;

unsigned int current_seed = 0;
int current_rand = 0;

FunctionPointer(void,	_srand,	(int seed),	0x007A89C6);
FunctionPointer(int,	_rand,	(void),		0x007A89D8);

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

struct lol { char byte0; };

DataPointer(lol*, EmeraldManagerObj2, 0x01AF014C);

int __cdecl hook_rand()
{
	using namespace Globals;

	if (!isConnected() || GameState < GameState::LoadFinished)
		return rand();

	if (EmeraldManagerObj2 != nullptr && EmeraldManagerObj2->byte0 != 3)
		return rand();

	if (Networking->isServer())
	{
		current_rand = rand();

		Broker->Request(MessageID::S_Rand, true);
		Broker->Finalize();

		Broker->SendReady(MessageID::S_Rand);
		Broker->WaitForPlayers(MessageID::S_Rand);

		return current_rand;
	}
	else
	{
		if (Broker->WaitForPlayers(MessageID::S_Rand))
		{
			Broker->SendReady(MessageID::S_Rand);
			return current_rand;
		}
		else
		{
			return rand();
		}
	}
}

void InitRandom()
{
	return;
	WriteCall((void*)0x0073939B, hook_rand);
	WriteCall((void*)0x0073881A, hook_rand);
	WriteCall((void*)0x007380F1, hook_rand);
}

//Trampoline srand_t(0x007A89C6, 0x007A89CB, (DetourFunction)hook_srand);
//Trampoline rand_t(0x007A89D8, 0x007A89DD, (DetourFunction)hook_rand);
