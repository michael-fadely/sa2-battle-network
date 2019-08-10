#pragma once

#include <SA2ModLoader.h>
#include "typedefs.h"

#pragma pack(push, 1)
struct PolarCoord
{
	Angle angle;
	float distance;
};
#pragma pack(pop)

DataArray(int, RumblePort_B, 0x008ACF78, 4);
