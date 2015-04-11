#include "Globals.h"

namespace sa2bn
{
	namespace Globals
	{
		HANDLE ProcessID = 0;
		PacketHandler* Networking = nullptr;
		PacketBroker* Broker = nullptr;
		::Program* Program = nullptr;
	}
}