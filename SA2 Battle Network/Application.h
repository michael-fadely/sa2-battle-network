#pragma once

#include <string>
#include "Networking.h"

class PacketHandler;
class QSocket;

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
		friend class PacketHandler;

	public:
		// De/Constructor
		Program(bool server, clientAddress& address, Settings& settings, unsigned int timeout);
		~Program();

		// Methods
		//bool isProcessRunning();

		ExitCode Connect();
		void Disconnect(bool received, ExitCode code = ExitCode::ClientDisconnect);

		void ApplySettings();
		ExitCode RunLoop();

		bool OnEnd();

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
		// Methods:
		void SendInitMsg();

		// Members
		ExitCode exitCode;
		QSocket* Socket;
		clientAddress	Address;

		PacketHandler* Networking;

		//HANDLE ProcessID;
		Settings settings;

	};
}