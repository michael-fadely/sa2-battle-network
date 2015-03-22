// Fixes PROCESS_ALL_ACCESS for Windows XP
#define _WIN32_WINNT 0x501

#define MODEXPORT __declspec(dllexport)

#include <iostream>
#include <string>

#include <Windows.h>		// for GetCommandLIneW(), GetCurrentProcess()
#include <ShellAPI.h>		// for CommandLinetoArgvW
#include <SA2ModLoader.h>

#include "Common.h"			// for LazyTypedefs, SleepFor, Globals
#include "Networking.h"		// for MSG_COUNT
#include "PacketHandler.h"	// for RemoteAddress
#include "Program.h"		// for Program

#include "MainThread.h"

// Namespaces
using namespace std;
using namespace chrono;

// Globals
int argc = 0;
wchar_t** argv = nullptr;

extern "C"
{
	MODEXPORT ModInfo SA2ModInfo = { ModLoaderVer };

	void MODEXPORT __cdecl Init(const char *path)
	{
		if (setvbuf(stdout, 0, _IOLBF, 4096) != 0)
			abort();
		if (setvbuf(stderr, 0, _IOLBF, 4096) != 0)
			abort();

		argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		thread mainThread(MainThread, argc, argv);
		mainThread.detach();
		return;
	}
}

void MainThread(int argc, wchar_t** argv)
{
	bool ValidArguments = false;
	bool isServer = false;
	uint timeout = 15000;

	PacketHandler::RemoteAddress Address;
	Program::Settings Settings = {};

#pragma region Command line arguments
	if (argc > 2)
	{
		for (int i = 1; i < argc; i++)
		{
			// Connection
			if ((!wcscmp(argv[i], L"--host") || !wcscmp(argv[i], L"-h")) && (i + 1) < argc)
			{
				isServer = true;
				//netMode = "server";
				Address.port = _wtoi(argv[++i]);
				ValidArguments = true;
			}
			else if ((!wcscmp(argv[i], L"--connect") || !wcscmp(argv[i], L"-c")) && (i + 2) < argc)
			{
				isServer = false;

				wstring wstr = argv[++i];
				string sstr(wstr.begin(), wstr.end());

				Address.ip = sstr;
				Address.port = _wtoi(argv[++i]);
				ValidArguments = true;
			}
			else if ((!wcscmp(argv[i], L"--timeout") || !wcscmp(argv[i], L"-t")) && (i + 1) < argc)
			{
				if ((timeout = _wtoi(argv[++i])) < 1000)
					timeout = 1000;

				ValidArguments = true;
			}

			// Configuration
			else if (!wcscmp(argv[i], L"--no-specials"))
			{
				Settings.noSpecials = true;
				ValidArguments = true;
			}
			else if (!wcscmp(argv[i], L"--local") || !wcscmp(argv[i], L"-l"))
			{
				Settings.isLocal = true;
				ValidArguments = true;
			}
			else if (!wcscmp(argv[i], L"--keep-active"))
			{
				Settings.KeepWindowActive = true;
				ValidArguments = true;
			}
		}
	}
	else
	{
		return;
	}

	if (!ValidArguments)
		return;
#pragma endregion

	PacketEx::SetMessageTypeCount(MSG_COUNT);
	sa2bn::Globals::ProcessID = GetCurrentProcess();
	sa2bn::Globals::Networking = new PacketHandler();
	Program* program = new Program(Settings, isServer, Address);

	while (true)
	{
		if (program->Connect() != Program::ExitCode::NotReady)
			program->RunLoop();

		SleepFor((milliseconds)250);
	}

	delete program;
	delete sa2bn::Globals::Networking;
}
