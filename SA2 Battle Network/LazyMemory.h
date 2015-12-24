#pragma once

#include <Windows.h>
#include "Globals.h"

inline SIZE_T ReadMemory(const SIZE_T baseAddress, void* buffer, const SIZE_T nSize)
{
	SIZE_T returnSize;
	ReadProcessMemory(GetCurrentProcess(), (void*)baseAddress, (LPCVOID*)buffer, nSize, &returnSize);
	return returnSize;
}

inline SIZE_T WriteMemory(const SIZE_T baseAddress, void* buffer, const SIZE_T nSize)
{
	SIZE_T returnSize;
	WriteProcessMemory(GetCurrentProcess(), (void*)baseAddress, (LPCVOID*)buffer, nSize, &returnSize);
	return returnSize;
}
