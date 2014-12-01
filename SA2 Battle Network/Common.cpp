#include <Windows.h>

#include "Common.h"

const unsigned int millisecs()
{
	return GetTickCount();
}
const unsigned int Duration(unsigned int timer)
{
	return (unsigned int)(GetTickCount() - timer);
}