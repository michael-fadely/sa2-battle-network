#pragma once
#include "Trampoline.h"

//extern Trampoline srand_t;
//extern Trampoline rand_t;
extern unsigned int current_seed;
extern int current_rand;

void InitRandom();

inline void srand_Original(unsigned int seed)
{
	return;
	//FunctionPointer(void, _srand, (unsigned int seed), srand_t.Target());
	//_srand(seed);
}
