#include "Globals.h"

namespace sa2bn
{
	namespace Globals
	{
		HANDLE ProcessID = nullptr;
		PacketHandler* Networking = nullptr;
		PacketBroker* Broker = nullptr;
		::Program* Program = nullptr;
	}
}