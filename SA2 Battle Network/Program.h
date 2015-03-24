#pragma once

#include <string>
#include "PacketHandler.h"

// TODO: Rewrite a majority of this class.
class Program
{
public:
#pragma region Embedded Types
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

	enum class ErrorCode
	{
		None,
		NotReady,
		VersionMismatch,
		GameTerminated,
		ClientTimeout,
		ClientDisconnect,
	};
#pragma endregion

	Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address);

	bool CheckConnectOK();
	ErrorCode Connect();
	void Disconnect(const bool received, const ErrorCode code = ErrorCode::ClientDisconnect);

	bool isServer;
	static Version versionNum;
	static const std::string version;

	Version remoteVersion;

private:
	ErrorCode exitCode;

	Settings clientSettings;
	PacketHandler::RemoteAddress Address;

	bool setMusic;

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void ApplySettings(const bool apply);
};
