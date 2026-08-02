#pragma once
#include <cstdint>

/* -----------------------------------------------------------------------
 * This table is meant to be populated from Il2CppDumper's output
 * (dump.cs field listing),
 * NOT guessed. That's the whole point: these offsets come from the
 * game's own IL2CPP metadata, so they're exact for the version you
 * dumped against.
 *
 * Workflow per game update:
 *   1. Re-run Il2CppDumper against the new GameAssembly.dll +
 *      global-metadata.dat.
 *   2. Diff the new dump.cs field offsets for the relevant classes
 *      against this table (or re-run the extractor script below).
 *   3. Update the constants here, rebuild.
 *
 * This is still a manual step per update, but it's "copy a known-good
 * number from a tool" rather than "manually rediscover it in Cheat
 * Engine by hand" -- and it's exact, not heuristic, so OffsetResolver's
 * fallback search becomes a safety net rather than the primary method.
 * ----------------------------------------------------------------------

 * CONFIRMED from real il2cpp dump.cs (TaskbarHero.Data.HeroInfoData,
 * TypeDefIndex 1560). These are the STATIC / BASE / designer-authored
 * values -- ints, not floats. This is NOT the buffed/runtime stats array
 * the original CE script pulled from (that one read vtSingle floats out
 * of a separate 50-200 entry array reached via a different pointer
 * chain). Use these for base-stat display; keep looking for the runtime
 * stat class (likely an enum-keyed StatType -> float structure) for the
 * actual equipped/buffed values.
*/
struct HeroInfoDataOffsets
{
  static constexpr int32_t HeroKey = 0x30;  // int
  static constexpr int32_t HeroNameKey =
    0x38;  // string  <- matches original script's possibleHI+0x38 read
  static constexpr int32_t DescriptionKey     = 0x40;  // string
  static constexpr int32_t ClassType          = 0x48;  // enum EEquipClassType
  static constexpr int32_t MainWeaponGearType = 0x4C;  // enum EGearType
  static constexpr int32_t SubWeaponGearType  = 0x50;  // enum EGearType
  static constexpr int32_t SkillKey           = 0x54;  // int
  static constexpr int32_t AttackDamage       = 0x58;  // int (base, not runtime float)
  static constexpr int32_t AttackSpeed        = 0x5C;  // int (base)
  static constexpr int32_t CastSpeed          = 0x60;  // int (base)
  static constexpr int32_t CriticalChance     = 0x64;  // int (base)
  static constexpr int32_t CriticalDamage     = 0x68;  // int (base)
  static constexpr int32_t CooldownReduction  = 0x6C;  // int (base)
  static constexpr int32_t MaxHp              = 0x70;  // int (base)
  static constexpr int32_t Armor              = 0x74;  // int (base)
  static constexpr int32_t MovementSpeed      = 0x78;  // int (base)
  static constexpr int32_t UnlockCost         = 0x7C;  // int
  static constexpr int32_t IsMeleeHero        = 0x80;  // bool
  static constexpr int32_t IsAvailable        = 0x81;  // bool
  static constexpr int32_t IsFirstAvailable   = 0x82;  // bool
};

/* CONFIRMED end-to-end chain from real il2cpp dump.cs. Every offset below
 * is now DERIVED, not guessed -- see HeroStatsFinder README for the full
 * class-layout proof.
 *
 * HeroInfoData (TypeDefIndex 1560): static/base hero definition
 *   +0x30  HeroKey        (int)   -- matches original AOB ID-scan target
 *   +0x38  HeroNameKey    (string)
 *
 * vo (TypeDefIndex 979, abstract base class):
 *   +0x10  ze* <bfjz>k__BackingField   -- THE runtime stat container
 *   +0x18  Action
 *   +0x20  Action<StatType>
 *   +0x28  Action<float,float>
 *
 * vh : vo (TypeDefIndex 964, derived -- own fields start right after vo's):
 *   +0x30  HeroInfoData* bfhq   -- pointer back to the static definition
 *   +0x88  Hero* bfib
 *
 * ze (TypeDefIndex 1244, the stat container found via vh+0x10):
 *   +0x18  Dictionary<StatType,float>* bgac  -- verify in-game which of
 *   +0x20  Dictionary<StatType,float>* bgad     these is current vs base
 *
 * Discovery flow (still two AOB scans, but every downstream offset is exact):
 *   1. Scan for HeroInfoData's ID pattern -> get HeroInfoData instance address.
 *   2. Scan for a pointer TO that address -> that pointer lives at vh+0x30,
 *      so (matchAddr - 0x30) = vh instance base.
 *   3. Read (vhBase + 0x10) -> ze* (the stat container, no further guessing).
 *   4. Read (zePtr + 0x18) or (zePtr + 0x20) -> Dictionary<StatType,float>*.
 *   5. Walk the dictionary with Il2CppStatDictionary (see that header).
*/
struct Il2CppOffsets
{
  // HeroInfoData (static/base data) -- confirmed
  static constexpr int32_t HeroInfoData_HeroKey     = 0x30;
  static constexpr int32_t HeroInfoData_HeroNameKey = 0x38;

  // vo base class (inherited into vh) -- confirmed
  static constexpr int32_t Vo_StatContainer = 0x10;  // -> ze*

  // vh derived class -- confirmed
  static constexpr int32_t Vh_HeroInfoDataRef = 0x30;  // -> HeroInfoData*
  static constexpr int32_t Vh_HeroBackRef     = 0x88;  // -> Hero*

  // ze stat container -- confirmed field offsets, semantic meaning
  // (current vs base) still needs one in-game sanity check
  static constexpr int32_t Ze_StatsDictA = 0x18;  // Dictionary<StatType,float>*
  static constexpr int32_t Ze_StatsDictB = 0x20;  // Dictionary<StatType,float>*
                                                  // (original CE script read THIS one)
};

// Global instance -- swap this out (or load from a JSON/config file at
// startup) each time you regenerate offsets from a new IL2CPP dump.
inline const Il2CppOffsets& CurrentOffsets()
{
  static Il2CppOffsets offsets;
  return offsets;
}