#pragma once

#include <Trampoline.h>

namespace nethax
{
	namespace events
	{
		extern Trampoline AddRingsHax;
		extern int DirtyRingHack;

		static inline void AddRingsOriginal(char playerNum, int numRings)
		{
			void* target = AddRingsHax.Target();
			__asm
			{
				mov edx, [numRings]
				mov al, [playerNum]
				call target
			}
		}

	}
}
