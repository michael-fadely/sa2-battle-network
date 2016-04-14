#pragma once
#include <Trampoline.h>

namespace nethax
{
	namespace random
	{
		extern Trampoline srand_trampoline;
		extern unsigned int current_seed;
		
		void __cdecl srand_hook(unsigned int seed);
		inline void srand_original(unsigned int seed)
		{
			FunctionPointer(void, target, (unsigned int seed), srand_trampoline.Target());
			target(seed);
		}
	}
}