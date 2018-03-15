#pragma once

#include <WinCrypt.h>
#include <vector>
#include <string>

class Hash
{
public:	
	/**
	 * \brief 
	 * Initializes a new instance of the \sa Hash class.
	 * \param dwProvType Provider type. See "Cryptographic Provider Types" on MSDN for details.
	 * \param dwFlags See the MSDN definition of CryptAcquireContext for details.
	 */
	Hash(DWORD dwProvType = PROV_RSA_AES, DWORD dwFlags = CRYPT_VERIFYCONTEXT);
	~Hash();

	std::vector<uint8_t> compute_hash(const void* data, size_t size, ALG_ID kind) const;
	static std::string to_string(std::vector<uint8_t>& hash);
	static std::vector<uint8_t> from_string(const std::string& str);

private:
	HCRYPTPROV csp = 0;
};
