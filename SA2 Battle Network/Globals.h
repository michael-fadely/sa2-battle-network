#pragma once

#include "PacketHandler.h"
#include "MemoryHandler.h"
#include <WTypes.h>

namespace sa2bn
{
	class Globals
	{
	public:
		static HANDLE ProcessID;
		static PacketHandler* Networking;
		static MemoryHandler* Memory;

		static bool isConnected() { return Networking != nullptr && Memory != nullptr && Networking->isConnected(); }
	};
}