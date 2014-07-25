#pragma once

#include <Windows.h>
#include <LazyTypedefs.h>
#include "Globals.h"

inline const uint ReadMemory(const uint baseAddress, void* buffer, const SIZE_T nSize)
{
	uint returnSize;
	ReadProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, (SIZE_T*)&returnSize);
	return returnSize;
}

inline const uint WriteMemory(const uint baseAddress, void* buffer, const SIZE_T nSize)
{
	uint returnSize;
	WriteProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, (SIZE_T*)&returnSize);
	return returnSize;
}
