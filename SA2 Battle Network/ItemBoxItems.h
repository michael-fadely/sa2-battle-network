#pragma once

#include <SA2ModLoader/SA2Structs.h>
#include <ModLoader/MemAccess.h>

// TODO: Mod Loader
#pragma pack(push, 1)
struct ItemBoxItem
{
	int Texture;
	void(__cdecl *Code)(ObjectMaster *, int);
};
#pragma pack(pop)

DataArray(ItemBoxItem, SGItemBox_Items, 0x00B0AFA0, 11);
DataArray(ItemBoxItem, ItemBoxBalloon_Items, 0x00B19928, 11);
DataArray(ItemBoxItem, ItemBoxAir_Items, 0x00B493A0, 11);
DataArray(ItemBoxItem, ItemBox_Items, 0x00B4D120, 11);

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
