#include "stdafx.h"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>

#include <Windows.h>  // for GetCommandLineW()
#include <ShellAPI.h> // for CommandLineToArgvW()

#include <SA2ModLoader.h>

#include "typedefs.h"
#include "globals.h"
#include "OnGameState.h"
#include "Hash.h"

#include "INIFile.h"

static void store_paths(const char* path_relative);
static void fake_main(int argc, wchar_t** argv);

extern "C"
{
__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 };

__declspec(dllexport) void __cdecl Init(const char* path)
{
	store_paths(path);

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	fake_main(argc, argv);
	LocalFree(argv);
}
}

static std::string working_directory;
static std::string mod_path_relative;
static std::string mod_path_absolute;

static void store_paths(const char* path_relative)
{
	mod_path_relative = path_relative;

	const DWORD required_length = GetCurrentDirectoryA(0, nullptr);

	if (required_length < 1)
	{
		throw std::exception("GetCurrentDirectory failed");
	}

	working_directory.resize(required_length, '\0');

	const DWORD produced_length = GetCurrentDirectoryA(required_length, working_directory.data());

	if (produced_length != required_length - 1)
	{
		throw std::exception("GetCurrentDirectory didn't produce the expected number of characters");
	}

	working_directory.resize(produced_length);
	mod_path_absolute = working_directory + "\\" + mod_path_relative;
}

static std::string build_mod_path(const std::string& base, const std::string& path)
{
	std::stringstream result;
	result << base << "\\" << path;
	return result.str();
}

static std::string build_mod_path_absolute(const std::string& path)
{
	return build_mod_path(mod_path_absolute, path);
}

static std::string build_mod_path_relative(const std::string& path)
{
	return build_mod_path(mod_path_relative, path);
}

static void parse_config(const std::string& path, Program::Settings& settings, sws::Address& address)
{
	INIFile ini;

	if (std::filesystem::exists(path))
	{
		std::fstream file(path, std::fstream::in);
		ini.read(file);
	}

	std::shared_ptr<INISection> section = ini.get_section("Config");

	if (section)
	{
		settings.nickname    = section->try_get<std::string>("Nickname").value_or(std::string());
		settings.no_specials = !section->try_get<bool>("Specials").value_or(true);
		settings.cheats      = section->try_get<bool>("Cheats").value_or(false);
	}

	section = ini.get_section("Server");

	if (section)
	{
		settings.server_name = section->try_get<std::string>("Name").value_or(std::string());
		address.port = section->try_get<sws::port_t>("Port").value_or(21790);

		std::optional<std::string> password = section->try_get<std::string>("Password");

		if (password.has_value() && !password.value().empty())
		{
			const bool password_hashed = section->try_get<bool>("PasswordHashed").value_or(false);
			const std::string& password_value = password.value();

			if (password_hashed)
			{
				settings.password = Hash::from_string(password_value);
			}
			else
			{
				Hash hash;
				settings.password = hash.compute_hash(password_value.data(), password_value.length(), CALG_SHA_256);
				section->set<std::string>("Password", Hash::to_string(settings.password));
				section->set<bool>("PasswordHashed", true);

				std::fstream file(path, std::fstream::out | std::fstream::trunc);
				ini.write(file);
			}
		}
	}
}

static std::string wstring_to_string(const std::wstring& wstr)
{
	const size_t wstr_length = wstr.size();

	if (!wstr_length)
	{
		return {};
	}

	if (wstr_length > static_cast<size_t>(std::numeric_limits<int>::max()))
	{
		throw std::exception("input string length too long");
	}

	const int wstr_int_length = static_cast<int>(wstr_length);

	const int required_length = WideCharToMultiByte(CP_UTF8,
	                                                WC_ERR_INVALID_CHARS,
	                                                wstr.data(),
	                                                wstr_int_length,
	                                                nullptr,
	                                                0,
	                                                NULL,
	                                                NULL);

	if (required_length < 1)
	{
		throw std::exception("buffer size determination for wstring to string conversion failed");
	}

	std::string str(required_length, '\0');

	const int produced_length = WideCharToMultiByte(CP_UTF8,
	                                                WC_ERR_INVALID_CHARS,
	                                                wstr.data(),
	                                                wstr_int_length,
	                                                str.data(),
	                                                required_length,
	                                                NULL,
	                                                NULL);

	if (produced_length != required_length)
	{
		throw std::exception("produced characters do not match buffer length");
	}

	return str;
}

static void fake_main(int argc, wchar_t** argv)
{
	bool valid_args = false;
	bool is_server = false;
	uint timeout = 15000;

	// This serves multiple purposes.
	// e.g: If connecting, it stores a password for the remote server if applicable.
	Program::Settings settings = {};

	sws::Address address;

	parse_config(build_mod_path_absolute("config.ini"), settings, address);

	// TODO: fix cases where valid_args would invalidate configuration read from disk.
	for (int i = 1; i < argc; i++)
	{
		if ((!wcscmp(argv[i], L"--host") || !wcscmp(argv[i], L"-h")) && i + 1 < argc)
		{
			address.port = static_cast<sws::port_t>(std::stoi(argv[++i]));
			valid_args = true;
			is_server = true;
		}
		else if ((!wcscmp(argv[i], L"--connect") || !wcscmp(argv[i], L"-c")) && i + 1 < argc)
		{
			std::string ip = wstring_to_string(argv[++i]);

			if (ip.empty())
			{
				continue;
			}

			// TODO: parse IPv6
			const auto colon = ip.find_first_of(':');
			const sws::port_t port = colon == std::string::npos ? 21790 : static_cast<sws::port_t>(std::stoi(ip.substr(colon + 1)));

			address.address = ip.substr(0, colon);
			address.port = port;
			valid_args = true;
			is_server = false;
		}
		else if ((!wcscmp(argv[i], L"--timeout") || !wcscmp(argv[i], L"-t")) && i + 1 < argc)
		{
			timeout = std::max(2500, std::stoi(argv[++i]));
			valid_args = true;
		}
		else if (!wcscmp(argv[i], L"--no-specials"))
		{
			settings.no_specials = true;
		}
		else if (!wcscmp(argv[i], L"--cheats"))
		{
			settings.cheats = true;
			valid_args = true;
		}
		else if (!wcscmp(argv[i], L"--password") && i + 1 < argc)
		{
			std::string password = wstring_to_string(argv[++i]);

			Hash hash;
			settings.password = hash.compute_hash(password.c_str(), password.length(), CALG_SHA_256);
		}
		else if (!wcscmp(argv[i], L"--netstat"))
		{
			settings.netstat = true;
		}
	}

	Program::apply_settings(settings);

	if (!valid_args)
	{
		if (argc < 2)
		{
			PrintDebug("[SA2:BN] Insufficient parameters.");
		}
		else
		{
			PrintDebug("[SA2:BN] Invalid parameters.");
		}

		return;
	}

	using namespace nethax;

	const std::vector<sws::Address> addresses = sws::Address::get_addresses(address.address.c_str(), address.port, sws::AddressFamily::inet);
	address = addresses[0];

	globals::program = new Program(settings, is_server, address);
	globals::broker  = new PacketBroker(timeout);

	events::InitOnGameState();
}
