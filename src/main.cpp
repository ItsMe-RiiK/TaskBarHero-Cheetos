#include "HeroFinderV2.h"
#include "ProcessMemory.h"

#include <iostream>

static const char* StatTypeName(StatType s)
{
  switch (s) {
  case StatType::AttackDamage :
    return "AttackDamage";
  case StatType::AttackSpeed :
    return "AttackSpeed";
  case StatType::CriticalChance :
    return "CriticalChance";
  case StatType::CriticalDamage :
    return "CriticalDamage";
  case StatType::MaxHp :
    return "MaxHp";
  case StatType::Armor :
    return "Armor";
  case StatType::MovementSpeed :
    return "MovementSpeed";
  case StatType::CooldownReduction :
    return "CooldownReduction";
  case StatType::CastSpeed :
    return "CastSpeed";
  default :
    return "Other";
  }
}

int main(int argc, char** argv)
{
  std::wstring exeName = L"TaskBarHero.exe";
  if (argc > 1) {
    std::string a = argv[1];
    exeName       = std::wstring(a.begin(), a.end());
  }

  ProcessMemory mem;
  if (!mem.AttachByName(exeName)) {
    std::wcout << L"[HeroStatsFinder] Could not attach to " << exeName
               << L" -- is it running? Run this as admin if needed." << std::endl;
    return 1;
  }

  std::cout << "[HeroStatsFinder] Attached, pid=" << mem.Pid() << ". Scanning..." << std::endl;

  HeroFinderV2 finder(mem);
  auto         results = finder.FindAll(
    /*useStatsDictB=*/true
  );  // matches original script's dict choice; flip to false to compare against the other dictionary

  std::cout << "[HeroStatsFinder] Found " << results.size() << "/" << HeroFinderV2::HeroMap.size()
            << " heroes.\n";

  for (const auto& hr : results) {
    std::cout << "\n== " << hr.name << " (id " << hr.id << ") ==\n"
              << "  HeroInfoData @ 0x" << std::hex << hr.heroInfoDataAddr << "\n"
              << "  vh instance  @ 0x" << hr.vhInstanceAddr << "\n"
              << "  ze instance  @ 0x" << hr.zeInstanceAddr << "\n"
              << "  stats dict   @ 0x" << hr.statsDictAddr << std::dec << "\n";

    // The specific stats from the original Lua script, and the new god-mode values to inject.
    // DPS is excluded per user request.
    struct CheatTarget
    {
      std::string name;
      StatType    type;
      float       cheatValue;
    };
    std::vector<CheatTarget> targetStats = {
      {"MHP", StatType::MaxHp, 999999999.0f},
      // {"DPS", StatType::AttackDamage, ...} // Skipped, keeping real value
      {"ATK_SPD", StatType::AttackSpeed, 999999999.0f},
      {"CRIT_CHANCE", StatType::CriticalChance, 1.0f},  // 100%
      {"CRIT_DMG", StatType::CriticalDamage, 999999999.0f},
      {"ARMOR", StatType::Armor, 999999999.0f},
      {"MOV_SPD", StatType::MovementSpeed, 999999999.0f}
    };

    for (const auto& target : targetStats) {
      auto it = hr.stats.find(target.type);
      if (it != hr.stats.end()) {
        float     oldVal = it->second.value;
        uintptr_t addr   = it->second.address;

        // Perform the actual memory write to inject our cheat value
        if (mem.WriteFloat(addr, target.cheatValue)) {
          float       displayOld = oldVal;
          float       displayNew = target.cheatValue;
          std::string suffix     = "";

          if (target.name == "MOV_SPD") {
            displayOld *= 100.0f;
            displayNew *= 100.0f;
          }
          if (target.name == "CRIT_CHANCE" || target.name == "CRIT_DMG") {
            displayOld *= 100.0f;
            displayNew *= 100.0f;
            suffix = "%";
          }

          std::cout << "  [" << hr.name << "] " << target.name << ": addr=0x" << std::hex << addr
                    << std::dec << " | OLD: " << displayOld << suffix << " -> NEW: " << displayNew
                    << suffix << " (SUCCESS)\n";
        }
        else {
          std::cout << "  [" << hr.name << "] " << target.name << ": addr=0x" << std::hex << addr
                    << std::dec << " (FAILED TO WRITE)\n";
        }
      }
    }
  }
  return 0;
}