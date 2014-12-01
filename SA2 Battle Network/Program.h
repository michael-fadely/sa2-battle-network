#pragma once

#include <string>
#include "PacketHandler.h"

class MemoryHandler;

// TODO: Rewrite a majority of this class.
class Program
{
public:
	struct Settings
	{
		bool noSpecials;
		bool isLocal;
		bool KeepWindowActive;
	};

	struct Version
	{
		unsigned char major;
		unsigned char minor;
		const std::string str();
	};

	enum class ExitCode
	{
		None,
		NotReady,
		VersionMismatch,
		GameTerminated,
		ClientTimeout,
		ClientDisconnect,
	};

	Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address);
	~Program();

	ExitCode Connect();
	void Disconnect(const bool received, const ExitCode code = ExitCode::ClientDisconnect);

	ExitCode RunLoop();

	bool isServer;
	static Version versionNum;
	static const std::string version;

	Version remoteVersion;

private:
	ExitCode exitCode;

	MemoryHandler* memory;

	Settings clientSettings;
	PacketHandler::RemoteAddress Address;

	bool setMusic;

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void ApplySettings(const bool apply);
};
