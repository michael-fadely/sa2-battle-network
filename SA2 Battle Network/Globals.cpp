#include "Globals.h"

using namespace sa2bn;

HANDLE Globals::ProcessID			= 0;
PacketHandler* Globals::Networking	= nullptr;
PacketBroker* Globals::Memory		= nullptr;
Program* Globals::Program			= nullptr;