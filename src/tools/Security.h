#pragma once
#include <string>

namespace Security {
  // Returns true if the file matches the expected SHA-256 hash (hex string).
  // If expectedHashHex is empty, returns true (for local builds where hash is unknown).
  bool VerifyFileHashSha256(const std::string& filePath, const std::string& expectedHashHex);
}  // namespace Security
