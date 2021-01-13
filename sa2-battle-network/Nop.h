#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

class Nop
{
	static std::unordered_map<intptr_t, std::vector<uint8_t>> backup_data;

public:
	// Applies NOP to address and creates a backup of the original data.
	// Returns true if applied successfully, and false if it has already been applied and there is a backup.
	// If skip_backup is true, the NOP instructions will be applied regardless of whether a backup exists,
	// and will not attempt to create another backup.
	static bool apply(intptr_t address, size_t length, bool skip_backup = false);
	// Restores original code to address.
	// Returns true on successful restore,and false if there was no backup found.
	// If skip_erase is true, the backup data will be restored without being erased.
	static bool restore(intptr_t address, bool skip_erase = false);
};
