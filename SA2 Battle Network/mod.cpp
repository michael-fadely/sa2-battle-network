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
#include "OnStageChange.h"

// Namespaces
using namespace std;
using namespace chrono;

extern "C"
{
	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer };
	__declspec(dllexport) void __cdecl Init(const char* path)
	{
		int argc = 0;
		wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		MainThread(argc, argv);
	}
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
			timeout = min(2500, _wtoi(argv[++i]));
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
	Globals::Broker = new PacketBroker(timeout);

	InitOnGameState();
	//InitOnInput();
	//InitOnFrame();
	InitOnStageChange();
}
