#include "stdafx.h"

#include <string>

#include <Windows.h>		// for GetCommandLIneW(), GetCurrentProcess()
#include <ShellAPI.h>		// for CommandLinetoArgvW
#include <SA2ModLoader.h>

#include "typedefs.h"
#include "Globals.h"		// PacketHandler, Program, PacketBroker
#include "PacketHandler.h"	// for RemoteAddress

#include "MainThread.h"

#include "OnGameState.h"
#include "OnStageChange.h"
#include "Hash.h"

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
		LocalFree(argv);
	}
}

void MainThread(int argc, wchar_t** argv)
{
	if (argc < 2)
	{
		PrintDebug("[SA2:BN] Insufficient parameters.");
		return;
	}

	bool validArguments = false;
	bool isServer = false;
	uint timeout = 15000;

	PacketHandler::RemoteAddress Address;
	Program::Settings settings = {};

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
			timeout = max(2500, _wtoi(argv[++i]));
			validArguments = true;
		}
		// Configuration
		else if (!wcscmp(argv[i], L"--no-specials"))
		{
			settings.noSpecials = true;
			MemManage::nop2PSpecials(true);
			validArguments = true;
		}
		else if (!wcscmp(argv[i], L"--cheats"))
		{
			settings.cheats = true;
			validArguments = true;
		}
		else if (!wcscmp(argv[i], L"--password") && ++i < argc)
		{
			wstring password_w(argv[i]);
			string password_a(password_w.begin(), password_w.end());

			Hash hash;
			settings.password = hash.ComputeHash(password_a.c_str(), password_a.length(), CALG_SHA_256);
		}
		else if (!wcscmp(argv[i], L"--local") || !wcscmp(argv[i], L"-l"))
		{
			settings.local = true;
			validArguments = true;
		}
		else if (!wcscmp(argv[i], L"--netstat"))
		{
			settings.netStat = true;
		}
	}

	if (!validArguments)
	{
		PrintDebug("[SA2:BN] Invalid parameters.");
		return;
	}
#pragma endregion

	using namespace nethax;

	Globals::Networking = new PacketHandler();
	Globals::Program = new Program(settings, isServer, Address);
	Globals::Broker = new PacketBroker(timeout);

	InitOnGameState();
}
