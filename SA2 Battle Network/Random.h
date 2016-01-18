#pragma once
#include "Trampoline.h"

extern Trampoline srand_t;
extern unsigned int current_seed;

inline void srand_Original(unsigned int seed)
{
	FunctionPointer(void, target, (unsigned int seed), srand_t.Target());
	target(seed);
}
