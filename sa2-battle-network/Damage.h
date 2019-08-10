#pragma once

#include <SA2Structs.h>
#include <ninja.h>

namespace nethax
{
	namespace events
	{
		Sint32 DamagePlayer_original(EntityData1* data1, CharObj2Base* data2);
		void KillPlayer_original(int playerNum);

		void InitDamage();
		void DeinitDamage();
	}
}
