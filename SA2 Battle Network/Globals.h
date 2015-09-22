#pragma once

#include "PacketHandler.h"
#include "PacketBroker.h"
#include "Program.h"
#include <WTypes.h>

namespace nethax
{
	namespace Globals
	{
		extern HANDLE			ProcessID;
		extern PacketHandler*	Networking;
		extern PacketBroker*	Broker;
		extern ::Program*		Program;

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