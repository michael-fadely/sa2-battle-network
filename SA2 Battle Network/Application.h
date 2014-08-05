#pragma once

#include <string>
#include "Networking.h"

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
		VersionMismatch,
		GameTerminated,
		ClientTimeout,
		ClientDisconnect,
	};

	class Program
	{
	public:
		// De/Constructor
		Program(const bool host, const Settings& settings);
		~Program();

		// Methods
		//bool isProcessRunning();

		ExitCode Connect();
		void Disconnect(const bool received, const ExitCode code = ExitCode::ClientDisconnect);

		void ApplySettings();
		const ExitCode RunLoop();

		const bool OnEnd();

		// Members
		bool isServer;
		bool isConnected;

		// The time in milliseconds when the
		// connection was successfully initiated.
		uint ConnectionStart;

		static Version versionNum;
		static const std::string version;

		Version remoteVersion;

	private:

		// Members
		ExitCode exitCode;

		PacketHandler* Networking;

		//HANDLE ProcessID;
		Settings clientSettings;

	};
}