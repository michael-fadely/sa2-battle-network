#pragma once

#include "typedefs.h"
#include <map>
#include <vector>

class nop
{
private:
	static std::map<size_t, std::vector<uint8>> backupData;

public:
	// Applies NOP to address and creates a backup of the original data.
	// Returns true if applied successfully, and false if it has already been applied and there is a backup.
	// If skipBackup is true, the NOP instructions will be applied regardless of whether a backup exists,
	// and will not attempt to create another backup.
	static bool apply(size_t address, size_t length, bool skipBackup = false);
	// Restores original code to address.
	// Returns true on successful restore,and false if there was no backup found.
	// If skipErase is true, the backup data will be restored without being erased.
	static bool restore(size_t address, bool skipErase = false);
};
