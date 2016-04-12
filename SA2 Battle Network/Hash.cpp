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
		throw;
}

Hash::~Hash()
{
	CryptReleaseContext(csp, 0);
}

std::vector<uchar> Hash::ComputeHash(void* data, size_t size, ALG_ID kind) const
{
	// TODO: Error checking
	HCRYPTHASH hHash = 0;
	if (CryptCreateHash(csp, kind, 0, 0, &hHash) == FALSE)
	{
		DWORD error = GetLastError();
		throw;
	}

	CryptHashData(hHash, (const BYTE*)data, (DWORD)size, 0);

	DWORD hashSize;
	DWORD bufferSize = sizeof(size_t);
	CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashSize, &bufferSize, 0);
	std::vector<uchar> result(hashSize);

	bufferSize = result.size();
	CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)result.data(), &bufferSize, 0);
	CryptDestroyHash(hHash);

	return result;
}

std::string Hash::toString(std::vector<uchar>& hash)
{
	std::stringstream result;
	for (auto& i : hash)
		result << std::hex << std::setw(2) << std::setfill('0') << (int)i;
	return result.str();
}

std::vector<uchar> Hash::fromString(const std::string& str)
{
	if ((str.length() % 2) != 0)
		throw;

	std::vector<uchar> result;

	for (size_t i = 0; i < str.length(); i += 2)
	{
		std::istringstream stream(str.substr(i, 2));
		int temp;
		stream >> std::hex >> temp;
		result.push_back(temp);
	}

	return result;
}
