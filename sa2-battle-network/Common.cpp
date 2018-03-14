#include "stdafx.h"

#include <Windows.h>
#include "Common.h"

uint Millisecs()
{
	return GetTickCount();
}
uint Duration(uint timer)
{
	return GetTickCount() - timer;
}