#pragma once

#include <Windows.h>
#include <LazyTypedefs.h>
#include "Globals.h"

inline uint ReadMemory(uint baseAddress, void* buffer, SIZE_T nSize)
{
	uint returnSize;
	ReadProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, (SIZE_T*)&returnSize);
	return returnSize;
}

inline uint WriteMemory(uint baseAddress, void* buffer, SIZE_T nSize)
{
	uint returnSize;
	WriteProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, (SIZE_T*)&returnSize);
	return returnSize;
}
