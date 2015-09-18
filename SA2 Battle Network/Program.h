#pragma once

#include <string>
#include "PacketHandler.h"

// TODO: Reconsider the necessity for this class.
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
		std::string str();
		bool operator==(const Version& value) const { return major == value.major && minor == value.minor; }
		bool operator!=(const Version& value) const { return !(*this == value); }
	};
#pragma endregion

	Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address);

	bool CheckConnectOK() const;
	bool Connect();
	void Disconnect(const bool received);

	Version remoteVersion;

	static Version versionNum;
	static const std::string version;

private:
	Settings clientSettings;
	PacketHandler::RemoteAddress Address;

	bool isServer;
	bool setMusic;
	bool rejected;	// Prevents connection spam upon rejection (client only)

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void ApplySettings(const bool apply);
	bool StartServer();
	bool StartClient();
};
