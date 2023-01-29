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
	};

	struct Version
	{
		uint8_t major;
		uint8_t minor;
		[[nodiscard]] std::string str() const;
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

	Program(const Settings& settings, bool is_server, const sws::Address& address);

	static bool can_connect();
	bool connect();
	void disconnect();
	static void apply_settings(const Settings& settings);
	Version remote_version_;

	static const Version version_num;
	static const std::string version_string;

	const Settings& settings() const { return is_server_ ? local_settings_ : remote_settings_; }

private:
	Settings local_settings_, remote_settings_;
	sws::Address server_address_;

	bool is_server_;
	bool set_music_;
	bool rejected_; // Prevents connection spam upon rejection (client only)

	// Applies code and other changes to memory.
	// If apply is false, then the changes are reverted.
	void apply_settings() const;
	ConnectStatus start_server();
	ConnectStatus start_client();
	pnum_t player_num_;
};
