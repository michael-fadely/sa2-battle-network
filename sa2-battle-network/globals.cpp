#include "stdafx.h"
#include "globals.h"

namespace nethax
{
	namespace globals
	{
		HANDLE         process_id = nullptr;
		PacketHandler* networking = nullptr;
		PacketBroker*  broker     = nullptr;
		Program*       program    = nullptr;
	}
}
