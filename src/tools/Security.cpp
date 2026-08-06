#include "Security.h"
#include <windows.h>
#include <wincrypt.h>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "advapi32.lib")

namespace Security {

  bool VerifyFileHashSha256(const std::string& filePath, const std::string& expectedHashHex)
  {
    if (expectedHashHex.empty()) {
      return true;  // Skip validation if no hash is provided
    }

    HCRYPTPROV    hProv   = 0;
    HCRYPTHASH    hHash   = 0;
    bool          bResult = false;
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
      return false;
    }

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
      return false;
    }

    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
      const int         bufSize = 1024 * 32;
      std::vector<BYTE> buffer(bufSize);
      bool              hashSuccess = true;

      while (file.read(reinterpret_cast<char*>(buffer.data()), bufSize)) {
        if (!CryptHashData(hHash, buffer.data(), (DWORD) file.gcount(), 0)) {
          hashSuccess = false;
          break;
        }
      }
      if (hashSuccess && file.gcount() > 0) {
        if (!CryptHashData(hHash, buffer.data(), (DWORD) file.gcount(), 0)) {
          hashSuccess = false;
        }
      }

      if (hashSuccess) {
        DWORD cbHash  = 0;
        DWORD dwCount = sizeof(DWORD);
        if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*) &cbHash, &dwCount, 0)) {
          std::vector<BYTE> hashData(cbHash);
          if (CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &cbHash, 0)) {
            std::ostringstream oss;
            for (DWORD i = 0; i < cbHash; i++) {
              oss << std::hex << std::setw(2) << std::setfill('0') << (int) hashData[i];
            }
            std::string actualHash = oss.str();

            // Case-insensitive comparison
            if (actualHash.size() == expectedHashHex.size()) {
              bResult = true;
              for (size_t i = 0; i < actualHash.size(); i++) {
                if (tolower(actualHash[i]) != tolower(expectedHashHex[i])) {
                  bResult = false;
                  break;
                }
              }
            }
          }
        }
      }
      CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);

    return bResult;
  }

}  // namespace Security
