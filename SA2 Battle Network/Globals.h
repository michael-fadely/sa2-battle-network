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
		static PacketBroker*	Broker;
		static Program*			Program;

		static bool isInitialized()
		{
			return (Networking != nullptr && Broker != nullptr && Program != nullptr);
		}
		static bool isConnected()
		{
			return isInitialized() && Networking->isConnected();
		}
	};
}