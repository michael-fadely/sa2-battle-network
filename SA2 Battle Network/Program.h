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
		bool operator==(const Version& value) { return major == value.major && minor == value.minor; }
		bool operator!=(const Version& value) { return !(*this == value); }
	};
#pragma endregion

	/// <summary>
	/// Initializes a new instance of the <see cref="Program"/> class.
	/// </summary>
	/// <param name="settings">The settings.</param>
	/// <param name="host">Indicates if this instance is a server or client.</param>
	/// <param name="address">The port to listen on if <paramref name="host"/> is true, otherwise the remote address to connect to.</param>
	Program(const Settings& settings, const bool host, PacketHandler::RemoteAddress address);

	/// <summary>
	/// Checks if it's safe to start the connection.
	/// </summary>
	/// <returns><c>true</c> if safe.</returns>
	bool CheckConnectOK();
	/// <summary>
	/// Attempts to connect in a non-blocking fashion.
	/// </summary>
	/// <returns><c>ErrorCode::None</c> on success.</returns>
	bool Connect();
	/// <summary>
	/// Closes all connections.
	/// </summary>
	/// <param name="received">If <c>true</c>, sends a message to all open connections notifying them of the disconnect.</param>
	/// <param name="code">The error code to set.</param>
	void Disconnect(const bool received);

	Version remoteVersion;

	static Version versionNum;
	static const std::string version;

private:
	Settings clientSettings;
	PacketHandler::RemoteAddress Address;

	bool isServer;
	bool setMusic;

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void ApplySettings(const bool apply);
};
