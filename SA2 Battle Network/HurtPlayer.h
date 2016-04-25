#pragma once

#include <SA2ModLoader/SA2Structs.h>
#include <ninja.h>

namespace nethax
{
	namespace events
	{
		Sint32 DamagePlayer_original(CharObj1* data1, CharObj2Base* data2);
		void HurtPlayer_original(int playerNum);
		void KillPlayer_original(int playerNum);

		void InitHurtPlayer();
		void DeinitHurtPlayer();
	}
}
