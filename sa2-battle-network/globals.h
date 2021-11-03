#pragma once

#include "PacketBroker.h"
#include "Program.h"

namespace nethax
{
	namespace globals
	{
		extern PacketBroker*  broker;
		extern Program*       program;

		inline bool is_initialized()
		{
			return broker != nullptr && program != nullptr;
		}

		inline bool is_connected()
		{
			return is_initialized() && broker->is_connected();
		}
	};
}
