#include <vector>
#include <map>

#include <LazyTypedefs.h>
#include "LazyMemory.h"

#include "nop.h"

using namespace std;

map<size_t, vector<uint8>> nop::backupData;

bool nop::apply(size_t address, size_t length, bool skipBackup)
{
	// If there's already backup data for this address, return false
	// unless the skipBackup override is enabled.
	if (!skipBackup && backupData.find(address) != backupData.end())
		return false;

	// Populate an array with "length" NOP instructions
	vector<uint8> nop(length, 0x90);

	// If skipBackup isn't enabled, backup the original data.
	if (!skipBackup)
	{
		vector<uint8> code(length);
		ReadMemory(address, code.data(), length);
		backupData[address] = code;
	}

	// Write the NOP instructions to memory.
	WriteMemory(address, nop.data(), length);

	return true;
}

bool nop::restore(size_t address, bool skipErase)
{
	auto it = backupData.find(address);
	// If no backup data was found for this address, return false.
	if (it == backupData.end())
		return false;

	// Otherwise, restore the backup data to memory.
	WriteMemory(address, it->second.data(), it->second.size());

	// If skipErase enabled, do not erase the backup data.
	if (!skipErase)
		backupData.erase(address);

	return true;
}