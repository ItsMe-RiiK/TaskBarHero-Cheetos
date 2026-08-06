#pragma once
#include "../core/ProcessMemory.h"
#include "../scanner/PlayerDataFinder.h"

#include <string>
#include <vector>
#include <map>

/* -----------------------------------------------------------------------
 * Rune Unlocker — unlocks (if fresh start account) and levels up all runes in the rune tree.
 *
 * Runes are stored in PlayerSaveData -> List<RuneSaveData> at offset 0x90.
 * Each RuneSaveData has:
 *   0x10: int RuneKey
 *   0x14: int Level   (0 = locked, >0 = unlocked+leveled)
 *
 * Strategy: iterate the flat list and set Level to the target value
 * for all entries.
 * ----------------------------------------------------------------------- */
class RuneUnlocker
{
public:
  explicit RuneUnlocker(ProcessMemory& mem) :
      m_mem(mem)
  {
  }

  void SetRuneListAddr(uintptr_t addr) { m_runeListAddr = addr; }
  bool HasRuneList() const { return m_runeListAddr != 0; }

  // Show current rune status
  std::string DumpRunes(PlayerDataFinder& finder);

  const std::vector<RuneSaveInfo>&
  ScanRunes(PlayerDataFinder& finder, const std::map<int, int>& maxLevels);

  // Upgrades a specific rune by +addAmount
  std::string UpgradeRune(int32_t runeKey, int32_t addAmount);

  // Unlocks all locked runes (sets Level 0 -> 1)
  std::string UnlockAllLocked();

  // Upgrades ALL unlocked runes by +addAmount
  std::string UpgradeAllUnlocked(int32_t addAmount);

  // Instantly upgrades ALL unlocked runes to their true Max Level
  std::string UpgradeAllToMax();

  const std::vector<RuneSaveInfo>& GetCachedRunes() const { return m_runes; }

  int GetMaxLevelForRune(int runeKey) const;

private:
  ProcessMemory&            m_mem;
  uintptr_t                 m_runeListAddr = 0;
  std::vector<RuneSaveInfo> m_runes;
  std::map<int, int>        m_runeMaxLevels;
};
