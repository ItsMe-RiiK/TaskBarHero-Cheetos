#pragma once

#include "ProcessMemory.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// Pattern element: value + whether it's a wildcard ("??" in CE's AOB syntax)
struct PatternByte
{
  uint8_t value    = 0;
  bool    wildcard = false;
};

// Parses a CE-style AOB string, e.g. "00 00 ?? ?? 8C 01" -> vector<PatternByte>
inline std::vector<PatternByte> ParsePattern(const std::string& aob)
{
  std::vector<PatternByte> pattern;
  std::istringstream       iss(aob);
  std::string              tok;
  while (iss >> tok) {
    PatternByte pb;
    if (tok == "??" || tok == "?") {
      pb.wildcard = true;
    }
    else {
      pb.value = (uint8_t) std::stoul(tok, nullptr, 16);
    }
    pattern.push_back(pb);
  }
  return pattern;
}

// Equivalent to CE's AOBScan(pattern, "+W-C"): scans committed/readable
// (optionally writable) regions of the target process for a byte pattern.
class AOBScanner
{
public:
  explicit AOBScanner(const ProcessMemory& mem) :
      m_mem(mem)
  {
  }

  // requireWritable mirrors "+W" in CE's protection flags string.
  std::vector<uintptr_t>
  Scan(const std::vector<PatternByte>& pattern, bool requireWritable = true) const
  {
    std::vector<uintptr_t> hits;
    if (pattern.empty())
      return hits;

    auto                 regions = m_mem.EnumerateRegions(requireWritable);
    std::vector<uint8_t> buf;

    for (const auto& mbi : regions) {
      size_t regionSize = mbi.RegionSize;
      if (regionSize == 0 || regionSize > (256ull * 1024 * 1024))
        continue;  // skip absurd regions
      buf.resize(regionSize);

      SIZE_T bytesRead = 0;
      if (
        !ReadProcessMemory(m_mem.Handle(), mbi.BaseAddress, buf.data(), regionSize, &bytesRead)
        || bytesRead == 0
      ) {
        continue;
      }
      buf.resize(bytesRead);

      if (buf.size() < pattern.size())
        continue;

      size_t last = buf.size() - pattern.size();
      for (size_t i = 0; i <= last; i++) {
        bool match = true;
        for (size_t j = 0; j < pattern.size(); j++) {
          if (!pattern[j].wildcard && buf[i + j] != pattern[j].value) {
            match = false;
            break;
          }
        }
        if (match) {
          hits.push_back((uintptr_t) mbi.BaseAddress + i);
        }
      }
    }
    return hits;
  }

  std::vector<uintptr_t> Scan(const std::string& aob, bool requireWritable = true) const
  {
    return Scan(ParsePattern(aob), requireWritable);
  }

private:
  const ProcessMemory& m_mem;
};

// Builds the little-endian byte pattern for a pointer value, e.g. for
// re-scanning "who points at this address" (what the Lua script does with
// ptrBytes / hiAddr).
inline std::string PointerToPatternString(uintptr_t ptr)
{
  std::ostringstream oss;
  for (int i = 0; i < 8; i++) {
    uint8_t b = (uint8_t) ((ptr >> (i * 8)) & 0xFF);
    char    buf[4];
    snprintf(buf, sizeof(buf), "%02X ", b);
    oss << buf;
  }
  return oss.str();
}

// Builds the little-endian byte pattern for a 32-bit ID value embedded in a
// wider zero-padded field (mirrors idBytes in the Lua script:
// "00 00 00 00 00 00 00 00 XX XX XX XX 00 00 00 00").
inline std::string IdToPatternString(uint32_t id)
{
  std::ostringstream oss;
  oss << "00 00 00 00 00 00 00 00 ";
  for (int i = 0; i < 4; i++) {
    uint8_t b = (uint8_t) ((id >> (i * 8)) & 0xFF);
    char    buf[4];
    snprintf(buf, sizeof(buf), "%02X ", b);
    oss << buf;
  }
  oss << "00 00 00 00";
  return oss.str();
}