#pragma once

namespace nethax
{
	namespace events
	{
		void AddRings_SyncToggle(bool value);
		void AddRings_original(char playerNum, int numRings);
		void InitAddRings();
		void DeinitAddRings();
	}
}
