#pragma once

#ifdef _DEBUG

#include <SA2ModLoader.h>
#include <Trampoline.h>

#include <sws/SocketError.h>
#include <sws/Address.h>
#include <sws/Socket.h>
#include <sws/TcpSocket.h>
#include <sws/UdpSocket.h>
#include <sws/Packet.h>

#include <ShellAPI.h>
#include <Windows.h>
#include <WinCrypt.h>
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
#include "CommonEnums.h"
#include "EmeraldSync.h"
#include "Events.h"
#include "globals.h"
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
#include "chao.h"

#endif
