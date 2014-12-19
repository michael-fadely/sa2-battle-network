#include <Windows.h>

#include "Common.h"

const uint64 millisecs()
{
	return GetTickCount64();
}
const uint64 Duration(uint64 timer)
{
	return (unsigned int)(GetTickCount64() - timer);
}