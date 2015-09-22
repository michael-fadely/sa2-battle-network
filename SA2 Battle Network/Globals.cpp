#include "Globals.h"

namespace nethax
{
	namespace Globals
	{
		HANDLE ProcessID = nullptr;
		PacketHandler* Networking = nullptr;
		PacketBroker* Broker = nullptr;
		::Program* Program = nullptr;
	}
}