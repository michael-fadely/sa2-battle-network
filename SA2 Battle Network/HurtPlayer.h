#pragma once

namespace nethax
{
	namespace events
	{
		void KillPlayerOriginal(int playerNum);
		void HurtPlayerOriginal(int playerNum);

		void InitHurtPlayer();
		void DeinitHurtPlayer();
	}
}
