// Fixes PROCESS_ALL_ACCESS for Windows XP
#define _WIN32_WINNT 0x501

#include <string>

#include <Windows.h>		// for GetCommandLIneW(), GetCurrentProcess()
#include <ShellAPI.h>		// for CommandLinetoArgvW
#include <SA2ModLoader.h>

#include <LazyTypedefs.h>

#include "Globals.h"		// PacketHandler, Program, PacketBroker
#include "Networking.h"		// for MSG_COUNT
#include "PacketHandler.h"	// for RemoteAddress

#include "MainThread.h"

#include "OnInput.h"
#include "OnFrame.h"
#include "OnGameState.h"

// Namespaces
using namespace std;
using namespace chrono;

extern "C"				// Required for proper export
__declspec(dllexport)	// This data is being exported from this DLL
ModInfo SA2ModInfo = {
	ModLoaderVer,		// Struct version
	ThreadInit,			// Initialization function
	NULL, 0,			// List of Patches & Patch Count
	NULL, 0,			// List of Jumps & Jump Count
	NULL, 0,			// List of Calls & Call Count
	NULL, 0,			// List of Pointers & Pointer Count
};

// Globals
int argc = 0;
wchar_t** argv = nullptr;

void __cdecl ThreadInit(const char* path)
{
	if (setvbuf(stdout, 0, _IOLBF, 4096) != 0)
		abort();
	if (setvbuf(stderr, 0, _IOLBF, 4096) != 0)
		abort();

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	MainThread(argc, argv);
}

void MainThread(int argc, wchar_t** argv)
{
	if (argc < 3)
	{
		PrintDebug("[SA2:BN] Insufficient parameters.");
		return;
	}

	bool validArguments = false;
	bool isServer = false;
	uint timeout = 15000;

	PacketHandler::RemoteAddress Address;
	Program::Settings Settings = {};

#pragma region Command line arguments
	for (int i = 1; i < argc; i++)
	{
		// Connection
		if ((!wcscmp(argv[i], L"--host") || !wcscmp(argv[i], L"-h")) && (i + 1) < argc)
		{
			isServer = true;
			Address.port = _wtoi(argv[++i]);
			validArguments = true;
		}
		else if ((!wcscmp(argv[i], L"--connect") || !wcscmp(argv[i], L"-c")) && (i + 2) < argc)
		{
			isServer = false;

			wstring wstr = argv[++i];
			string sstr(wstr.begin(), wstr.end());

			Address.ip = sstr;
			Address.port = _wtoi(argv[++i]);
			validArguments = true;
		}
		else if ((!wcscmp(argv[i], L"--timeout") || !wcscmp(argv[i], L"-t")) && (i + 1) < argc)
		{
			if ((timeout = _wtoi(argv[++i])) < 1000)
				timeout = 1000;

			validArguments = true;
		}
		// Configuration
		else if (!wcscmp(argv[i], L"--no-specials"))
		{
			Settings.noSpecials = true;
			validArguments = true;
		}
		else if (!wcscmp(argv[i], L"--local") || !wcscmp(argv[i], L"-l"))
		{
			Settings.isLocal = true;
			validArguments = true;
		}
		else if (!wcscmp(argv[i], L"--keep-active"))
		{
			Settings.KeepWindowActive = true;
			validArguments = true;
		}
	}

	if (!validArguments)
	{
		PrintDebug("[SA2:BN] Invalid parameters.");
		return;
	}
#pragma endregion

	using namespace sa2bn;
	PacketEx::SetMessageTypeCount(MSG_COUNT);
	Globals::ProcessID = GetCurrentProcess();
	Globals::Networking = new PacketHandler();
	Globals::Program = new Program(Settings, isServer, Address);
	Globals::Broker = new PacketBroker();

	InitOnGameState();
	InitOnInput();
	InitOnFrame();
}
