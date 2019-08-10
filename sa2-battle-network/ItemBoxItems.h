#pragma once

#include <SA2ModLoader.h>

namespace nethax
{
	namespace events
	{
		void __stdcall NBarrier_original(int pnum);
		void __stdcall Speedup_original(int pnum);
		void __stdcall TBarrier_original(int pnum);
		void __stdcall Invincibility_original(ObjectMaster* object, int pnum);
		void __cdecl DisplayItemBoxItem_original(int pnum, int tnum);

		void InitItemBoxItems();
		void DeinitItemBoxItems();
	}
}
