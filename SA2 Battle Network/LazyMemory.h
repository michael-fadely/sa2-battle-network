#pragma once

#include <Windows.h>
#include <LazyTypedefs.h>
#include "Globals.h"

inline const uint ReadMemory(const SIZE_T baseAddress, void* buffer, const SIZE_T nSize)
{
	SIZE_T returnSize;
	ReadProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, &returnSize);
	return returnSize;
}

inline const uint WriteMemory(const SIZE_T baseAddress, void* buffer, const SIZE_T nSize)
{
	SIZE_T returnSize;
	WriteProcessMemory(sa2bn::Globals::ProcessID, (void*) baseAddress, (LPCVOID*) buffer, nSize, &returnSize);
	return returnSize;
}
