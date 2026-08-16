#include "PlayerDataFinder.h"
#include "../core/Il2CppDictionaryReader.h"
#include "../core/Il2CppOffsets.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

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
  // Pattern: HeroKey (101 = 0x65), HeroLevel (4 bytes, ignore), IsUnLock (1 byte = 0x01), 7 bytes zero padding
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
        // Knight is the starting hero, so it's always at index 0 in the HeroSaveDatas array!
        for (int idx = 0; idx < 1; idx++) {
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
              if (!runeList) {  // Just ensure we could read the pointer, allow *runeList to be 0
                continue;
              }

              auto currSize = m_mem.ReadInt32(*currencyList + Il2CppListOffsets::Size);
              if (!currSize || *currSize < 0 || *currSize > 1000) {
                continue;
              }

              auto runeSize = m_mem.ReadInt32(*runeList + Il2CppListOffsets::Size);
              if (!runeSize || *runeSize < 1 || *runeSize > 1000)
                continue;

              auto runeItemsPtr = m_mem.ReadPointer(*runeList + Il2CppListOffsets::Items);
              if (!runeItemsPtr || *runeItemsPtr == 0)
                continue;

              auto firstRunePtr = m_mem.ReadPointer(*runeItemsPtr + Il2CppArrayOffsets::Data);
              if (!firstRunePtr || *firstRunePtr == 0)
                continue;

              auto firstRuneKey = m_mem.ReadInt32(*firstRunePtr + RuneSaveDataOffsets::RuneKey);
              if (!firstRuneKey || *firstRuneKey <= 0 || *firstRuneKey > 99999999)
                continue;

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

                // Scan the buffer for the integer (e.g. 197 or similar for runes)
                for (size_t i = Il2CppDictOffsets::Count;
                     reg.RegionSize >= 0x30 && i < reg.RegionSize - 0x30;
                     i += 4) {  // 4-byte aligned
                  int32_t val = *reinterpret_cast<int32_t*>(&buffer[i]);
                  // Accept a reasonable range of total runes just in case it's no longer exactly 197
                  if (val >= 100 && val <= 500) {
                    size_t dictOffset = i - Il2CppDictOffsets::Count;

                    // Ultra-fast local buffer heuristics before any RPC!
                    int32_t freeCount = *reinterpret_cast<int32_t*>(
                      &buffer[dictOffset + Il2CppDictOffsets::FreeCount]
                    );
                    if (freeCount < 0 || freeCount > 1000)
                      continue;

                    uintptr_t bucketsPtr = *reinterpret_cast<uintptr_t*>(
                      &buffer[dictOffset + Il2CppDictOffsets::Buckets]
                    );
                    uintptr_t entriesPtr = *reinterpret_cast<uintptr_t*>(
                      &buffer[dictOffset + Il2CppDictOffsets::Entries]
                    );

                    // Typical x64 user-space pointers are between 0x100000 and 0x7FFFFFFFFFFF
                    if (bucketsPtr < 0x100000 || bucketsPtr > 0x7FFFFFFFFFFF)
                      continue;
                    if (entriesPtr < 0x100000 || entriesPtr > 0x7FFFFFFFFFFF)
                      continue;

                    // Ultra-fast check if pointer is actually in a valid memory region using binary search
                    auto isValidPtr = [&](uintptr_t ptr) {
                      auto it = std::lower_bound(
                        regions.begin(), regions.end(), ptr,
                        [](const MEMORY_BASIC_INFORMATION& mbi, uintptr_t val) {
                          return (uintptr_t) mbi.BaseAddress + mbi.RegionSize <= val;
                        }
                      );
                      return it != regions.end() && ptr >= (uintptr_t) it->BaseAddress;
                    };

                    if (!isValidPtr(bucketsPtr) || !isValidPtr(entriesPtr)) {
                      continue;
                    }

                    uintptr_t dictAddr = (uintptr_t) reg.BaseAddress + dictOffset;

                    // Perform safety heuristics
                    if (entriesPtr == 0)
                      continue;

                    auto entriesLen = m_mem.ReadInt32(entriesPtr + Il2CppArrayOffsets::Length);
                    if (!entriesLen || *entriesLen < val || *entriesLen > 10000)
                      continue;

                    // Now let's try to read it as the outer dictionary
                    Il2CppDictionaryReader dictReader(m_mem);
                    auto                   outerEntries = dictReader.ReadOuterEntries(dictAddr);

                    if (outerEntries.size() == static_cast<size_t>(val)) {
                      // Validate that this dictionary contains a known valid rune key from our save data!
                      bool hasPlayerRune = false;
                      for (const auto& pair : outerEntries) {
                        if (pair.first == *firstRuneKey) {
                          hasPlayerRune = true;
                          break;
                        }
                      }
                      if (!hasPlayerRune)
                        continue;  // This is the wrong dictionary!

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
