#pragma once

namespace nethax
{
	namespace events
	{
		void AddRings_sync_toggle(bool value);
		void AddRings_original(int8_t player_num, int32_t num_rings);
		void InitAddRings();
		void DeinitAddRings();
	}
}
