#include "stdafx.h"

#include <Windows.h>
#include "Common.h"

uint32 Millisecs()
{
	return GetTickCount();
}
uint32 Duration(uint32 timer)
{
	return (GetTickCount() - timer);
}