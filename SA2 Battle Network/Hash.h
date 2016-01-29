#pragma once
#include <WinCrypt.h>
#include <vector>

// TODO: vectors

class Hash
{
public:	
	/// <summary>
	/// Initializes a new instance of the <see cref="Hash"/> class.
	/// </summary>
	/// <param name="dwProvType">Provider type. See "Cryptographic Provider Types" on MSDN for details.</param>
	/// <param name="dwFlags">See the MSDN definition of CryptAcquireContext for details.</param>
	Hash(DWORD dwProvType = PROV_RSA_AES, DWORD dwFlags = CRYPT_VERIFYCONTEXT);
	~Hash();

	std::vector<char> ComputeHash(const char* data, size_t size, ALG_ID kind);

private:
	HCRYPTPROV csp;
};
