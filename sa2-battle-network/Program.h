#pragma once

#include <string>

class Program
{
public:
#pragma region Embedded Types
	struct Settings
	{
		// Synchronized settings
		std::string nickname;
		std::string server_name;
		bool no_specials;
		bool cheats;
		std::vector<uint8_t> password;

		// Local settings
		bool netstat;
		bool local;
	};

	struct Version
	{
		uint8_t major;
		uint8_t minor;
		std::string str() const;
		bool operator==(const Version& value) const;
		bool operator!=(const Version& value) const;
	};

	enum class ConnectStatus
	{
		listening,
		success,
		error
	};
#pragma endregion

	Program(const Settings& settings, bool host, const sws::Address& address);

	static bool can_connect();
	bool connect();
	void disconnect();
	static void apply_settings(const Settings& settings);
	Version remote_version;

	static const Version version_num;
	static const std::string version;

	const Settings& settings() const { return is_server ? local_settings : remote_settings; }

private:
	Settings local_settings, remote_settings;
	sws::Address address;

	bool is_server;
	bool set_music;
	bool rejected;	// Prevents connection spam upon rejection (client only)

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void apply_settings() const;
	ConnectStatus start_server();
	ConnectStatus start_client();
	pnum_t player_num;
};
