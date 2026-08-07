#include "PlayerDataFinder.h"
#include "../core/Il2CppDictionaryReader.h"
#include "../core/Il2CppOffsets.h"
#include <cstdio>
#include <cstring>

std::optional<PlayerDataResult> PlayerDataFinder::Find()
{
  if (m_hasCache) {
    // Basic validation to ensure pointer is still valid
    auto vtable = m_mem.ReadPointer(m_cachedResult.playerSaveDataAddr);
    if (vtable && *vtable > 0x10000) {
      return m_cachedResult;
    }
    m_hasCache = false;
  }

  printf("  [Debug] Searching for Knight HeroSaveData pattern...\n");
  // Step 1: Find a HeroSaveData by scanning for hero key = 101 (Knight)
  // Pattern: HeroKey (101 = 0x65), HeroLevel (4 bytes, ignore), IsUnLock (1 byte = 0x01), 7 bytes padding
  std::string pattern = "65 00 00 00 ?? ?? ?? ?? 01 00 00 00 00 00 00 00";
  auto        hits    = m_scanner.Scan(pattern, true);

  printf("  [Debug] Found %zu potential HeroSaveData hits.\n", hits.size());
  int validHeroes = 0;

  for (auto addr : hits) {
    uintptr_t objBase = addr - HeroSaveDataOffsets::HeroKey;

    auto level = m_mem.ReadInt32(objBase + HeroSaveDataOffsets::HeroLevel);
    if (!level || *level < 1 || *level > 9999999)
      continue;

    auto unlocked = m_mem.ReadBool(objBase + HeroSaveDataOffsets::IsUnLock);
    if (!unlocked || !*unlocked)
      continue;

    auto exp = m_mem.ReadDouble(objBase + HeroSaveDataOffsets::HeroExp);
    if (!exp || *exp < 0.0)
      continue;

    auto vtable = m_mem.ReadPointer(objBase);
    if (!vtable || *vtable < 0x100000)
      continue;

    validHeroes++;
    uintptr_t knightSaveData = objBase;
    printf("  [Debug] Found valid Knight @ 0x%llX\n", (unsigned long long) knightSaveData);

    // Step 2: Scan for a pointer to this HeroSaveData instance
    std::string ptrPattern = PointerToPatternString(knightSaveData);
    auto        ptrHits    = m_scanner.Scan(ptrPattern, true);

    if (!ptrHits.empty()) {
      printf("  [Debug]   -> Found %zu pointers to Knight.\n", ptrHits.size());
      for (auto ptrAddr : ptrHits) {
        for (int idx = 0; idx < 6; idx++) {
          uintptr_t arrayBase = ptrAddr - Il2CppArrayOffsets::Data - (idx * sizeof(uintptr_t));
          if (arrayBase == 0)
            continue;

          auto arrLen = m_mem.ReadInt32(arrayBase + Il2CppArrayOffsets::Length);
          if (!arrLen || *arrLen < 1 || *arrLen > 1000)
            continue;

          std::string arrPtrPattern = PointerToPatternString(arrayBase);
          auto        arrPtrHits    = m_scanner.Scan(arrPtrPattern, true);

          for (auto listCandAddr : arrPtrHits) {
            uintptr_t listBase = listCandAddr - Il2CppListOffsets::Items;

            auto listSize = m_mem.ReadInt32(listBase + Il2CppListOffsets::Size);
            if (!listSize || *listSize < 1 || *listSize > 1000)
              continue;

            std::string listPtrPattern = PointerToPatternString(listBase);
            auto        listPtrHits    = m_scanner.Scan(listPtrPattern, true);

            for (auto psdCandAddr : listPtrHits) {
              uintptr_t psdBase = psdCandAddr - PlayerSaveDataOffsets::HeroSaveDatas;

              auto currencyList =
                m_mem.ReadPointer(psdBase + PlayerSaveDataOffsets::CurrencySaveDatas);
              auto heroList = m_mem.ReadPointer(psdBase + PlayerSaveDataOffsets::HeroSaveDatas);
              auto runeList = m_mem.ReadPointer(psdBase + PlayerSaveDataOffsets::RuneSaveData);

              if (!currencyList || *currencyList == 0) {
                continue;
              }
              if (!runeList || *runeList == 0) {
                continue;
              }

              auto currSize = m_mem.ReadInt32(*currencyList + Il2CppListOffsets::Size);
              if (!currSize || *currSize < 0 || *currSize > 1000) {
                continue;
              }

              printf(
                "  [Debug]           -> SUCCESS! PlayerSaveData found @ 0x%llX\n",
                (unsigned long long) psdBase
              );
              PlayerDataResult result;
              result.playerSaveDataAddr = psdBase;
              result.heroListAddr       = listBase;
              result.currencyListAddr   = *currencyList;
              result.runeListAddr       = *runeList;

              // --- Extract Rune Max Levels dynamically via Heuristic Heap Scan ---
              auto regions   = m_mem.EnumerateRegions(true);
              bool foundDict = false;

              for (const auto& reg : regions) {
                if (foundDict)
                  break;

                // Read the entire region into a buffer
                std::vector<uint8_t> buffer(reg.RegionSize);
                if (!m_mem.ReadBytes((uintptr_t) reg.BaseAddress, buffer.data(), reg.RegionSize))
                  continue;

                // Scan the buffer for the integer 197 (0x000000C5)
                for (size_t i = Il2CppDictOffsets::Count;
                     reg.RegionSize >= 0x30 && i < reg.RegionSize - 0x30;
                     i += 4) {  // 4-byte aligned
                  int32_t val = *reinterpret_cast<int32_t*>(&buffer[i]);
                  if (val == 197) {  // Found a potential dictionary count!
                    uintptr_t dictAddr = (uintptr_t) reg.BaseAddress + i - Il2CppDictOffsets::Count;

                    // Perform safety heuristics
                    auto entriesPtr = m_mem.ReadPointer(dictAddr + Il2CppDictOffsets::Entries);
                    if (!entriesPtr || *entriesPtr == 0)
                      continue;

                    auto freeCount = m_mem.ReadInt32(dictAddr + Il2CppDictOffsets::FreeCount);
                    if (!freeCount || *freeCount < 0 || *freeCount > 1000)
                      continue;

                    auto entriesLen = m_mem.ReadInt32(*entriesPtr + Il2CppArrayOffsets::Length);
                    if (!entriesLen || *entriesLen < 197 || *entriesLen > 10000)
                      continue;

                    // Now let's try to read it as the outer dictionary
                    Il2CppDictionaryReader dictReader(m_mem);
                    auto                   outerEntries = dictReader.ReadOuterEntries(dictAddr);

                    if (outerEntries.size() == 197) {
                      // Check if the keys look like rune keys (e.g. 101, 102... or > 0 at least)
                      // And the values must be valid inner dictionaries!
                      bool isValid         = true;
                      int  validInnerCount = 0;
                      for (size_t j = 0; j < 5; j++) {  // Check first 5 entries
                        if (outerEntries[j].first <= 0) {
                          isValid = false;
                          break;
                        }
                        auto innerDictPtr = outerEntries[j].second;
                        auto innerCount   = dictReader.GetCount(innerDictPtr);
                        if (!innerCount || *innerCount <= 0 || *innerCount > 20) {
                          isValid = false;
                          break;
                        }
                        validInnerCount++;
                      }

                      if (isValid && validInnerCount == 5) {
                        for (const auto& pair : outerEntries) {
                          int       runeKey      = pair.first;
                          uintptr_t innerDictPtr = pair.second;
                          auto      maxLevelOpt  = dictReader.GetCount(innerDictPtr);
                          if (maxLevelOpt) {
                            result.runeMaxLevels[runeKey] = *maxLevelOpt;
                          }
                        }
                        foundDict = true;
                        break;
                      }
                    }
                  }
                }
              }

              m_cachedResult = result;
              m_hasCache     = true;
              return result;
            }
          }
        }
      }
    }
    else {
      printf("  [Debug]   -> No pointers found to Knight.\n");
    }
  }
  printf("  [Debug] Checked %d valid heroes, but found no PlayerSaveData chain.\n", validHeroes);
  return std::nullopt;
}

std::vector<uintptr_t> PlayerDataFinder::FindCurrencySaveDatas(int32_t currencyKey)
{
  std::vector<uintptr_t> results;
  uint8_t*               kb = (uint8_t*) &currencyKey;

  // Pattern: Key (4 bytes), padding (4 bytes)
  char pattern[32];
  snprintf(pattern, sizeof(pattern), "%02X %02X %02X %02X 00 00 00 00", kb[0], kb[1], kb[2], kb[3]);

  auto hits = m_scanner.Scan(pattern, true);
  for (auto addr : hits) {
    uintptr_t objBase = addr - CurrencySaveDataOffsets::Key;  // offset of Key
    // Validate vtable
    auto vtable = m_mem.ReadPointer(objBase);
    if (!vtable || *vtable < 0x10000)
      continue;

    results.push_back(objBase);
  }
  return results;
}

std::vector<uintptr_t> PlayerDataFinder::FindObscuredLongByValue(int64_t exactValue)
{
  std::vector<uintptr_t> results;
  if (exactValue <= 1000)
    return results;  // too low, will match random garbage

  auto                 regions = m_mem.EnumerateRegions(false);
  std::vector<uint8_t> buffer;

  for (const auto& r : regions) {
    buffer.resize(r.RegionSize);
    if (m_mem.ReadBytes((uintptr_t) r.BaseAddress, buffer.data(), r.RegionSize)) {
      // Iterate in 8-byte steps (alignment of int64_t)
      for (size_t i = 0; i <= r.RegionSize - 16; i += 8) {
        int64_t hidden;
        int64_t key;
        memcpy(&hidden, &buffer[i], sizeof(int64_t));
        memcpy(&key, &buffer[i + 8], sizeof(int64_t));

        if ((hidden ^ key) == exactValue) {
          // i is the offset of 'hidden'
          // ObscuredLong starts at i - 8 (because hash is at 0, hidden is at 8)
          if (i >= 8) {
            uintptr_t baseAddr = (uintptr_t) r.BaseAddress + i - 8;
            int32_t   hash;
            memcpy(&hash, &buffer[i - 8], sizeof(int32_t));

            // Compute expected hash to ensure this is actually an ObscuredLong
            // Hash logic: (int)(value ^ (value >> 32))
            int32_t expectedHash = (int32_t) (exactValue ^ (exactValue >> 32));
            if (hash == expectedHash) {
              results.push_back(baseAddr);
            }
          }
        }
      }
    }
  }
  return results;
}

std::vector<CurrencyInfo> PlayerDataFinder::ReadCurrencies(uintptr_t currencyListAddr)
{
  std::vector<CurrencyInfo> result;
  auto                      elements = m_listReader.ReadElementPointers(currencyListAddr);

  for (auto elemAddr : elements) {
    CurrencyInfo ci;
    ci.addr = elemAddr;

    auto key = m_mem.ReadInt32(elemAddr + CurrencySaveDataOffsets::Key);
    if (!key)
      continue;
    ci.key = *key;

    auto qty = m_mem.ReadInt64(elemAddr + CurrencySaveDataOffsets::Quantity);
    if (!qty)
      continue;
    ci.quantity = *qty;

    result.push_back(ci);
  }
  return result;
}

std::vector<HeroSaveInfo> PlayerDataFinder::ReadHeroes(uintptr_t heroListAddr)
{
  std::vector<HeroSaveInfo> result;
  auto                      elements = m_listReader.ReadElementPointers(heroListAddr);

  for (auto elemAddr : elements) {
    HeroSaveInfo hi;
    hi.addr = elemAddr;

    auto key = m_mem.ReadInt32(elemAddr + HeroSaveDataOffsets::HeroKey);
    if (!key)
      continue;
    hi.heroKey = *key;

    auto level = m_mem.ReadInt32(elemAddr + HeroSaveDataOffsets::HeroLevel);
    if (level)
      hi.level = *level;

    auto unlocked = m_mem.ReadBool(elemAddr + HeroSaveDataOffsets::IsUnLock);
    if (unlocked)
      hi.unlocked = *unlocked;

    auto exp = m_mem.ReadDouble(elemAddr + HeroSaveDataOffsets::HeroExp);
    if (exp)
      hi.exp = *exp;

    result.push_back(hi);
  }
  return result;
}

std::vector<RuneSaveInfo> PlayerDataFinder::ReadRunes(uintptr_t runeListAddr)
{
  std::vector<RuneSaveInfo> result;
  auto                      elements = m_listReader.ReadElementPointers(runeListAddr);

  for (auto elemAddr : elements) {
    RuneSaveInfo ri;
    ri.addr = elemAddr;

    auto key = m_mem.ReadInt32(elemAddr + RuneSaveDataOffsets::RuneKey);
    if (!key)
      continue;
    ri.runeKey = *key;

    auto level = m_mem.ReadInt32(elemAddr + RuneSaveDataOffsets::Level);
    if (level)
      ri.level = *level;

    result.push_back(ri);
  }
  return result;
}
