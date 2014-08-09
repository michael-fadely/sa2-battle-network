// Fixes PROCESS_ALL_ACCESS for Windows XP
#define _WIN32_WINNT 0x501

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Common.h"			// for LazyTypedefs, SleepFor, Globals
#include "Networking.h"		// for MSG_COUNT
#include "PacketHandler.h"	// for RemoteAddress
#include "Application.h"	// for Application

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
	string raw(GetCommandLineA());
	stringstream clean;

	for (uint i = 1; i < raw.length(); i++)
	{
		if (raw[i] == '\"')
		{
			for (uint x = (i + 2); x < raw.length(); x++)
				clean << raw[x];

			break;
		}
	}

	for (string s; getline(clean, s, ' ');)
		if (s.length() > 0)
			args.push_back(s);


	return (args.size() > 1);
}

void MainThread()
{
	bool isServer = false;
	uint timeout = 15000;

	PacketHandler::RemoteAddress Address;
	Application::Program* Program = nullptr;
	Application::Settings Settings = {};
	Application::ExitCode ExitCode;

#pragma region Command line arguments
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
				Address.port = atoi(args[++i].c_str());
			}
			else if ((args[i] == "--connect" || args[i] == "-c") && (i + 2) < argc)
			{
				isServer = false;
				//netMode = "client";
				Address.ip = args[++i];
				cout << Address.ip << endl;
				Address.port = atoi(args[++i].c_str());
			}
			else if ((args[i] == "--timeout" || args[i] == "-t") && (i + 1) < argc)
			{
				if ((timeout = atoi(args[++i].c_str())) < 1000)
					timeout = 1000;
			}

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


	PacketEx::SetMessageTypeCount(MSG_COUNT);
	sa2bn::Globals::ProcessID = GetCurrentProcess();
	sa2bn::Globals::Networking = new PacketHandler();

	while (true)
	{
		Program = new Application::Program(Settings, isServer, Address);

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
