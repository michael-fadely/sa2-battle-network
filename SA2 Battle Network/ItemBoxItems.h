#pragma once

#include <SA2ModLoader/SA2Structs.h>

// TODO: Mod Loader
#pragma pack(push, 1)
struct ItemBoxItem
{
	int Texture;
	void(__cdecl *Code)(ObjectMaster *, int);
};
#pragma pack(pop)

DataArray(ItemBoxItem, ItemBoxItems_A, 0x00B0AFA0, 11);
DataArray(ItemBoxItem, ItemBoxItems_B, 0x00B19928, 11);
DataArray(ItemBoxItem, ItemBoxItems_C, 0x00B493A0, 11);
DataArray(ItemBoxItem, ItemBoxItems_D, 0x00B4D120, 11);

namespace nethax
{
	namespace events
	{
		void __cdecl NBarrier_original(ObjectMaster* object, int pnum);
		void __cdecl Speedup_original(ObjectMaster* object, int pnum);
		void __cdecl TBarrier_original(ObjectMaster* object, int pnum);
		void __cdecl Invincibility_original(ObjectMaster* object, int pnum);

		void InitItemBoxItems();
		void DeinitItemBoxItems();
	}
}
