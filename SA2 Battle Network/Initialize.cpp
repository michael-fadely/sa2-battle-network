// Fixes PROCESS_ALL_ACCESS for Windows XP
#define _WIN32_WINNT 0x501

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"
#include "Networking.h"
#include "PacketExtensions.h"
#include "Application.h"

#include "Initialize.h"

// Prototypes
void hprint(std::string text, std::chrono::milliseconds sleep);

// Namespaces
using namespace std;
using namespace chrono;

// Globals
vector<string> args;

void __cdecl Init_t(const char *path)
{
	thread mainThread(MainThread);
	mainThread.detach();
	return;
}

const bool CommandLine()
{
	stringstream CommandLine(GetCommandLineA());

	for (string s; getline(CommandLine, s, ' ');)
		args.push_back(s);

	return (args.size() > 1);
}

void MainThread()
{
	bool isServer = false;
	uint timeout = 15000;

	Application::Program* Program;
	Application::Settings Settings = {};
	Application::ExitCode ExitCode;

	//SetConsoleTitleA(Application::Program::version.c_str());
	//hprint("Thanks for trying Sonic Adventure 2: Battle Network Alpha!\n", (milliseconds)25);
	//SleepFor((milliseconds)250);

#pragma region "Command line" arguments
	if (CommandLine())
	{
		uint argc = args.size();
		for (uint i = 0; i < argc; i++)
		{
			// Connection
			if ((args[i] == "--host" || args[i] == "-h") && (i + 1) < argc)
			{
				isServer = true;
				//netMode = "server";
				addrStruct.port = atoi(args[++i].c_str());
			}
			else if ((args[i] == "--connect" || args[i] == "-c") && (i + 2) < argc)
			{
				isServer = false;
				//netMode = "client";
				addrStruct.address = args[++i];
				addrStruct.port = atoi(args[++i].c_str());
			}
			else if ((args[i] == "--timeout" || args[i] == "-t") && (i + 1) < argc)
				timeout = atoi(args[++i].c_str());

			// Configuration
			else if (args[i] == "--no-specials")
				Settings.noSpecials = true;
			else if (args[i] == "--local" || args[i] == "-l")
				Settings.isLocal = true;
			else if (args[i] == "--keep-active")
				Settings.KeepWindowActive = true;
		}
	}
	else
	{
		return;
	}
#pragma endregion

	if (timeout < 1000)
		timeout = 1000;

	PacketEx::SetMessageTypeCount(MSG_COUNT);

	while (true)
	{
		// Find the SA2 process
		//cout << "Looking for SA2 window...\nPlease launch SA2 to start shenanigans.\n";
		sa2bn::Globals::ProcessID = GetCurrentProcess();

		Program = new Application::Program(isServer, addrStruct, Settings, timeout);

		Program->Connect();
		Program->ApplySettings();
		// This will run indefinitely unless something stops it from
		// the inside. Therefore, we delete immediately after.
		ExitCode = Program->RunLoop();

		bool result = Program->OnEnd();
		delete Program;

		if (!result)
			break;
		else
			cout << "<> Reinitializing..." << endl;

	}

	cout << "Thanks for testing!" << endl;
	SleepFor((milliseconds)750);
	return;
}

// Unnecessary haxy print
void hprint(std::string text, std::chrono::milliseconds sleep)
{
	for (unsigned int i = 0; i < text.length(); i++)
	{
		cout << text[i];
		SleepFor(sleep);
	}
}
