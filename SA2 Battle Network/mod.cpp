#include "stdafx.h"

#include <string>
#include <Windows.h>		// for GetCommandLineW(), GetCurrentProcess()
#include <ShellAPI.h>		// for CommandLinetoArgvW
#include <direct.h>			// for _getcwd
#include <SA2ModLoader.h>

#include "typedefs.h"
#include "Globals.h"		// PacketHandler, Program, PacketBroker
#include "PacketHandler.h"	// for RemoteAddress
#include "OnGameState.h"
#include "PoseEffect2PStartMan.h"
#include "ItemBoxItems.h"
#include "CharacterSync.h"
#include "Hash.h"

void MainThread(const char* path, int argc, wchar_t** argv);

extern "C"
{
	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer };
	__declspec(dllexport) void __cdecl Init(const char* path)
	{
		int argc = 0;
		wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		MainThread(path, argc, argv);
		LocalFree(argv);
	}
}

std::string BuildModPath(const char* modpath, const char* path)
{
	std::stringstream result;
	char workingdir[FILENAME_MAX];

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\" << path;

	return result.str();
}

void ParseConfig(const std::string& path, Program::Settings& settings, PacketHandler::RemoteAddress& address)
{
	char buffer[255];

	GetPrivateProfileStringA("Config", "Nickname", "", buffer, 255, path.c_str());
	if (strlen(buffer))
		settings.nickname = buffer;

	settings.noSpecials = GetPrivateProfileIntA("Config", "Specials", 1, path.c_str()) == 0;
	settings.cheats = GetPrivateProfileIntA("Config", "Cheats", 0, path.c_str()) != 0;

	GetPrivateProfileStringA("Server", "Name", "", buffer, 255, path.c_str());
	if (strlen(buffer))
		settings.serverName = buffer;

	address.port = (ushort)GetPrivateProfileIntA("Server", "Port", 21790, path.c_str());

	GetPrivateProfileStringA("Server", "Password", "", buffer, 255, path.c_str());
	if (strlen(buffer))
	{
		std::string password_a(buffer);

		if (GetPrivateProfileIntA("Server", "PasswordHashed", 0, path.c_str()) != 1)
		{
			Hash hash;
			settings.password = hash.ComputeHash((void*)password_a.c_str(), password_a.length(), CALG_SHA_256);
			WritePrivateProfileStringA("Server", "Password", Hash::toString(settings.password).c_str(), path.c_str());
			WritePrivateProfileStringA("Server", "PasswordHashed", "1", path.c_str());
		}
		else
		{
			settings.password = Hash::fromString(password_a);
		}
	}
}

void MainThread(const char* path, int argc, wchar_t** argv)
{
	bool valid_args = false;
	bool is_server = false;
	uint timeout = 15000;

	// This serves multiple purposes.
	// e.g: If connecting, it stores a password for the remote server if applicable.
	Program::Settings settings = {};
	PacketHandler::RemoteAddress address;

	ParseConfig(BuildModPath(path, "config.ini"), settings, address);

	// TODO: fix cases where valid_args would invalidate configuration read from disk.
	for (int i = 1; i < argc; i++)
	{
		// Connection
		if ((!wcscmp(argv[i], L"--host") || !wcscmp(argv[i], L"-h")) && i + 1 < argc)
		{
			address.port = std::stoi(argv[++i]);
			valid_args = true;
			is_server = true;
		}
		else if ((!wcscmp(argv[i], L"--connect") || !wcscmp(argv[i], L"-c")) && i + 1 < argc)
		{
			std::wstring ip_w = argv[++i];
			std::string ip(ip_w.begin(), ip_w.end());

			if (ip.empty())
				continue;

			const auto npos = ip.npos;
			auto colon = ip.find_first_of(':');
			ushort port = colon == npos ? 21790 : (ushort)std::stoi(ip.substr(colon + 1));

			address.ip = ip.substr(0, colon);
			address.port = port;
			valid_args = true;
			is_server = false;
		}
		else if ((!wcscmp(argv[i], L"--timeout") || !wcscmp(argv[i], L"-t")) && i + 1 < argc)
		{
			timeout = max(2500, std::stoi(argv[++i]));
			valid_args = true;
		}
		// Configuration
		else if (!wcscmp(argv[i], L"--no-specials"))
		{
			settings.noSpecials = true;
			valid_args = true;
		}
		else if (!wcscmp(argv[i], L"--cheats"))
		{
			settings.cheats = true;
			valid_args = true;
		}
		else if (!wcscmp(argv[i], L"--password") && ++i < argc)
		{
			std::wstring password_w(argv[i]);
			std::string password_a(password_w.begin(), password_w.end());

			Hash hash;
			settings.password = hash.ComputeHash((void*)password_a.c_str(), password_a.length(), CALG_SHA_256);
		}
		else if (!wcscmp(argv[i], L"--local") || !wcscmp(argv[i], L"-l"))
		{
			settings.local = true;
			valid_args = true;
		}
		else if (!wcscmp(argv[i], L"--netstat"))
		{
			settings.netStat = true;
		}
	}

	Program::ApplySettings(settings);

	if (!valid_args)
	{
		if (argc < 2)
			PrintDebug("[SA2:BN] Insufficient parameters.");
		else
			PrintDebug("[SA2:BN] Invalid parameters.");

		return;
	}

	using namespace nethax;

	Globals::Networking = new PacketHandler();
	Globals::Program = new Program(settings, is_server, address);
	Globals::Broker = new PacketBroker(timeout);

	events::InitOnGameState();
	events::InitPoseEffect2PStartMan();
	events::InitItemBoxItems();
	events::InitCharacterSync();
}
