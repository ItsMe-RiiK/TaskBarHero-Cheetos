#pragma once
#include "../core/AOBScanner.h"
#include "../core/Il2CppListReader.h"
#include "../core/ProcessMemory.h"

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

/* -----------------------------------------------------------------------
 * Finds the PlayerSaveData instance in memory by locating known
 * HeroSaveData objects and tracing the pointer chain back up.
 * 
 *   1. Scan for HeroSaveData instances by looking for known hero key
 *      patterns (101, 201, 301, etc.) at the expected offset.
 *   2. Scan for pointers to those HeroSaveData instances → these live
 *      in the backing array of List<HeroSaveData>.
 *   3. Scan for pointers to the backing array → these are List._items.
 *   4. The List pointer at PlayerSaveData+0x70 points to the List.
 *
 * Once PlayerSaveData is found, all sub-lists (currency, runes, etc.)
 * can be accessed via known offsets.
 * ----------------------------------------------------------------------- */

struct PlayerDataResult
{
  uintptr_t          playerSaveDataAddr = 0;
  uintptr_t          heroListAddr       = 0;
  uintptr_t          currencyListAddr   = 0;
  uintptr_t          runeListAddr       = 0;
  std::map<int, int> runeMaxLevels;
};

struct CurrencyInfo
{
  uintptr_t addr     = 0;  // Address of the CurrencySaveData instance
  int32_t   key      = 0;
  int64_t   quantity = 0;
};

struct HeroSaveInfo
{
  uintptr_t addr     = 0;
  int32_t   heroKey  = 0;
  int32_t   level    = 0;
  bool      unlocked = false;
  double    exp      = 0.0;
};

struct RuneSaveInfo
{
  uintptr_t addr    = 0;
  int32_t   runeKey = 0;
  int32_t   level   = 0;
};

class PlayerDataFinder
{
public:
  explicit PlayerDataFinder(ProcessMemory& mem) :
      m_mem(mem),
      m_scanner(mem),
      m_listReader(mem)
  {
  }

  // Find PlayerSaveData by locating a known HeroSaveData and tracing back.
  std::optional<PlayerDataResult> Find();

  void ClearCache() { m_hasCache = false; }

  // Find ALL CurrencySaveData instances for a specific key
  std::vector<uintptr_t> FindCurrencySaveDatas(int32_t currencyKey);

  // Bruteforce search for an ObscuredLong by its exact decrypted value
  std::vector<uintptr_t> FindObscuredLongByValue(int64_t exactValue);

  // ---------------------------------------------------------------------------
  // Hero Scanner
  // ---------------------------------------------------------------------------

  std::vector<CurrencyInfo> ReadCurrencies(uintptr_t currencyListAddr);

  std::vector<HeroSaveInfo> ReadHeroes(uintptr_t heroListAddr);

  std::vector<RuneSaveInfo> ReadRunes(uintptr_t runeListAddr);

private:
  ProcessMemory&   m_mem;
  AOBScanner       m_scanner;
  Il2CppListReader m_listReader;

  bool             m_hasCache = false;
  PlayerDataResult m_cachedResult;
};
