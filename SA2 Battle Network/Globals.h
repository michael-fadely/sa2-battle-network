#pragma once

#include "PacketHandler.h"
#include "PacketBroker.h"
#include "Program.h"
#include <WTypes.h>

namespace sa2bn
{
	class Globals
	{
	public:
		static HANDLE			ProcessID;
		static PacketHandler*	Networking;
		static PacketBroker*	Memory;
		static Program*			Program;

		static bool isConnected()
		{
			return (Networking != nullptr && Memory != nullptr && Program != nullptr) && Networking->isConnected();
		}
	};
}