#include "stdafx.h"

#include <WinCrypt.h>
#include <vector>

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

std::vector<char> Hash::ComputeHash(const char* data, size_t size, ALG_ID kind)
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
	std::vector<char> result(hashSize);

	bufferSize = result.size();
	CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)result.data(), &bufferSize, 0);
	CryptDestroyHash(hHash);

	return result;
}
