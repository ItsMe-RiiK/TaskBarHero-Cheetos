#pragma once
#include "AOBScanner.h"
#include "Il2CppOffsets.h"
#include "Il2CppStatDictionary.h"
#include "ProcessMemory.h"
#include "StatType.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

struct HeroResultV2
{
  int         id = 0;
  std::string name;
  uintptr_t   heroInfoDataAddr = 0;
  uintptr_t   vhInstanceAddr   = 0;
  uintptr_t   zeInstanceAddr   = 0;
  uintptr_t   statsDictAddr    = 0;  // whichever of Ze_StatsDictA/B we resolved to use
  std::unordered_map<StatType, Il2CppStatDictionary::StatData> stats;
};

/* -----------------------------------------------------------------------
 * Rewritten hero-finder using the fully confirmed IL2CPP chain:
 *
 *   HeroInfoData (found via ID pattern, same as original script Phase 1)
 *     <- pointer found at vh+0x30 (found via original script Phase 2 AOB)
 *   vh instance
 *     +0x10 (inherited from vo) -> ze* (stat container)
 *   ze instance
 *     +0x18 or +0x20 -> Dictionary<StatType,float>*
 *   Dictionary
 *     +0x18 -> entries array -> length at +0x18, data at +0x20, 16 bytes/entry
 *
 * The two AOB scans (ID pattern, then pointer-to-HeroInfoData) are still
 * needed to LOCATE instances at runtime -- IL2CPP doesn't give you a
 * static list of live object addresses, only their layout. What's
 * different from the original script is that every offset used after
 * locating the object is now exact, not guessed/validated-at-runtime.
 * -----------------------------------------------------------------------
 */
class HeroFinderV2
{
public:
  inline static const std::map<int, std::string> HeroMap = {
    {101, "Knight"}, {201, "Ranger"}, {301, "Sorcerer"},
    {401, "Priest"}, {501, "Hunter"}, {601, "Slayer"},
  };

  explicit HeroFinderV2(ProcessMemory& mem) :
      m_mem(mem),
      m_scanner(mem),
      m_statDict(mem)
  {
  }

  std::vector<HeroResultV2> FindAll(bool useStatsDictB = true)
  {
    std::vector<HeroResultV2> results;

    // ---- Phase 1: locate HeroInfoData instances by embedded ID ----
    std::map<int, uintptr_t> heroInfoAddrs;

    for (const auto& [heroId, heroName] : HeroMap) {
      std::string idPattern = IdToPatternString((uint32_t) heroId);
      auto        hits      = m_scanner.Scan(idPattern, true);

      for (auto matchAddr : hits) {
        // ID pattern's 8 zero-padding bytes precede HeroKey, and
        // HeroKey sits at HeroInfoData+0x30, so the struct base is
        // matchAddr - 0x28 (confirmed, see Il2CppOffsets.h notes).
        uintptr_t heroInfoBase = matchAddr - 0x28;

        auto strPtr = m_mem.ReadPointer(heroInfoBase + Il2CppOffsets::HeroInfoData_HeroNameKey);
        if (!strPtr || *strPtr == 0)
          continue;

        auto strLen = m_mem.ReadInt32(*strPtr + 0x10);
        if (!strLen || *strLen <= 8 || *strLen >= 30)
          continue;

        std::wstring s = m_mem.ReadUtf16(*strPtr + 0x14, (size_t) std::min<int32_t>(*strLen, 20));
        std::wstring expected = L"HeroName_" + std::to_wstring(heroId);

        if (s.find(expected) != std::wstring::npos) {
          heroInfoAddrs[heroId] = heroInfoBase;
          break;
        }
      }
    }

    // ---- Phase 2: find the vh instance referencing each HeroInfoData ----
    for (const auto& [heroId, hiAddr] : heroInfoAddrs) {
      std::string ptrPattern = PointerToPatternString(hiAddr);
      auto        hits       = m_scanner.Scan(ptrPattern, true);

      for (auto refAddr : hits) {
        // The pointer-to-HeroInfoData lives in vh.bfhq at +0x30.
        uintptr_t vhBase = refAddr - Il2CppOffsets::Vh_HeroInfoDataRef;

        // ---- Phase 3: read the ze* stat container (exact, no guessing) ----
        auto zePtr = m_mem.ReadPointer(vhBase + Il2CppOffsets::Vo_StatContainer);
        if (!zePtr || *zePtr == 0)
          continue;

        // ---- Phase 4: read one of the two stat dictionaries ----
        int32_t dictFieldOffset =
          useStatsDictB ? Il2CppOffsets::Ze_StatsDictB : Il2CppOffsets::Ze_StatsDictA;

        auto dictPtr = m_mem.ReadPointer(*zePtr + dictFieldOffset);
        if (!dictPtr || *dictPtr == 0)
          continue;

        HeroResultV2 hr;
        hr.id               = heroId;
        hr.name             = HeroMap.at(heroId);
        hr.heroInfoDataAddr = hiAddr;
        hr.vhInstanceAddr   = vhBase;
        hr.zeInstanceAddr   = *zePtr;
        hr.statsDictAddr    = *dictPtr;
        hr.stats            = m_statDict.ReadAll(*dictPtr);

        results.push_back(hr);
        break;  // first valid vh instance wins
      }
    }

    return results;
  }

private:
  ProcessMemory&       m_mem;
  AOBScanner           m_scanner;
  Il2CppStatDictionary m_statDict;
};