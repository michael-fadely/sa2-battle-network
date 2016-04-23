#pragma once

namespace nethax
{
	namespace random
	{
		extern unsigned int current_seed;
		
		void srand_original(unsigned int seed);
		void __cdecl srand_hook(unsigned int seed);

		void InitRandom();
		void DeinitRandom();
	}
}