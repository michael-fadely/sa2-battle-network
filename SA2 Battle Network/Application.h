#pragma once

#include <string>
#include "PacketHandler.h"

class MemoryHandler;

namespace Application
{
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

	class Program
	{
	public:
		// De/Constructor
		Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address);
		~Program();

		// Methods
		//bool isProcessRunning();

		ExitCode Connect();
		void Disconnect(const bool received, const ExitCode code = ExitCode::ClientDisconnect);

		void ApplySettings();
		const ExitCode RunLoop();

		// TODO: Remove this function as it's no longer required, and check RunLoop's return value instead.
		const bool OnEnd();

		// Members
		bool isServer;
		static Version versionNum;
		static const std::string version;

		Version remoteVersion;

	private:

		// Members
		ExitCode exitCode;

		MemoryHandler* AbstractMemory;

		Settings clientSettings;
		PacketHandler::RemoteAddress Address;

		bool setMusic;
	};
}