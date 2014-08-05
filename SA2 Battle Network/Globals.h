#pragma once

#include "PacketHandler.h"

namespace sa2bn
{
	typedef void *HANDLE;
	class Globals
	{
	public:
		static HANDLE ProcessID;
		static PacketHandler Networking;
	};
}