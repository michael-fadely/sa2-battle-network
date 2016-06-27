#pragma once

#ifdef _DEBUG

#include <SA2ModLoader.h>
#include <SFML/Network.hpp>
#include <Trampoline.h>

#include <ShellAPI.h>
#include <WinSock2.h>
#include <Windows.h>
#include <WinCrypt.h>
#include <Winsock2.h>
#include <direct.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <fstream>
#include <deque>

#include "AddRings.h"
#include "AddressList.h"
#include "AdventurePacketOverloads.h"
#include "CharacterSync.h"
#include "Common.h"
#include "CommonEnums.h"
#include "EmeraldSync.h"
#include "Events.h"
#include "Globals.h"
#include "Hash.h"
#include "Damage.h"
#include "ItemBoxItems.h"
#include "LazyMemory.h"
#include "MemoryManagement.h"
#include "MemoryStruct.h"
#include "ModLoaderExtensions.h"
#include "Networking.h"
#include "OnGameState.h"
#include "OnStageChange.h"
#include "OnResult.h"
#include "PacketBroker.h"
#include "PacketEx.h"
#include "PacketHandler.h"
#include "PacketOverloads.h"
#include "PlayerObject.h"
#include "Program.h"
#include "Random.h"
#include "nop.h"
#include "typedefs.h"

#endif
