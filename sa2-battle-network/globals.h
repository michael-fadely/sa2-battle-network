#pragma once

#include "PacketHandler.h"
#include "PacketBroker.h"
#include "Program.h"

namespace nethax
{
	namespace globals
	{
		extern PacketHandler* networking;
		extern PacketBroker*  broker;
		extern Program*       program;

		inline bool is_initialized()
		{
			return networking != nullptr && broker != nullptr && program != nullptr;
		}

		inline bool is_connected()
		{
			return is_initialized() && networking->is_connected();
		}
	};
}
