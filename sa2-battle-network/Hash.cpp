#include "stdafx.h"

#include <WTypes.h>
#include <WinCrypt.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include "typedefs.h"

#include "Hash.h"

Hash::Hash(DWORD dwProvType, DWORD dwFlags)
{
	if (CryptAcquireContext(&csp, nullptr, nullptr, dwProvType, dwFlags) != TRUE)
	{
		throw;
	}
}

Hash::~Hash()
{
	CryptReleaseContext(csp, 0);
}

std::vector<uint8_t> Hash::compute_hash(const void* data, size_t size, ALG_ID kind) const
{
	// TODO: Error checking
	HCRYPTHASH handle = 0;
	if (CryptCreateHash(csp, kind, 0, 0, &handle) == FALSE)
	{
		throw;
	}

	CryptHashData(handle, static_cast<const BYTE*>(data), static_cast<DWORD>(size), 0);

	DWORD hash_size;
	DWORD buffer_size = sizeof(size_t);
	CryptGetHashParam(handle, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hash_size), &buffer_size, 0);
	std::vector<uint8_t> result(hash_size);

	buffer_size = result.size();
	CryptGetHashParam(handle, HP_HASHVAL, static_cast<BYTE*>(result.data()), &buffer_size, 0);
	CryptDestroyHash(handle);

	return result;
}

std::string Hash::to_string(std::vector<uint8_t>& hash)
{
	std::stringstream result;

	for (auto& i : hash)
	{
		result << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
	}

	return result.str();
}

std::vector<uint8_t> Hash::from_string(const std::string& str)
{
	if ((str.length() % 2) != 0)
	{
		throw;
	}

	std::vector<uint8_t> result;

	for (size_t i = 0; i < str.length(); i += 2)
	{
		std::istringstream stream(str.substr(i, 2));
		int temp;
		stream >> std::hex >> temp;
		result.push_back(temp);
	}

	return result;
}
