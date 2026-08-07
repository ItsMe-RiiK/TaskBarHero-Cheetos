#pragma once
#include <cstdint>

// =========================================================================
// Runtime Hero Stats Chain
// =========================================================================

struct HeroInfoDataOffsets
{
  static constexpr int32_t HeroKey            = 0x30;  // @[HeroInfoData.HeroKey]
  static constexpr int32_t HeroNameKey        = 0x38;  // @[HeroInfoData.HeroNameKey]
  static constexpr int32_t DescriptionKey     = 0x40;  // @[HeroInfoData.DescriptionKey]
  static constexpr int32_t ClassType          = 0x48;  // @[HeroInfoData.ClassType]
  static constexpr int32_t MainWeaponGearType = 0x4C;  // @[HeroInfoData.MainWeaponGearType]
  static constexpr int32_t SubWeaponGearType  = 0x50;  // @[HeroInfoData.SubWeaponGearType]
  static constexpr int32_t SkillKey           = 0x54;  // @[HeroInfoData.SkillKey]
  static constexpr int32_t AttackDamage       = 0x58;  // @[HeroInfoData.AttackDamage]
  static constexpr int32_t AttackSpeed        = 0x5C;  // @[HeroInfoData.AttackSpeed]
  static constexpr int32_t CastSpeed          = 0x60;  // @[HeroInfoData.CastSpeed]
  static constexpr int32_t CriticalChance     = 0x64;  // @[HeroInfoData.CriticalChance]
  static constexpr int32_t CriticalDamage     = 0x68;  // @[HeroInfoData.CriticalDamage]
  static constexpr int32_t CooldownReduction  = 0x6C;  // @[HeroInfoData.CooldownReduction]
  static constexpr int32_t MaxHp              = 0x70;  // @[HeroInfoData.MaxHp]
  static constexpr int32_t Armor              = 0x74;  // @[HeroInfoData.Armor]
  static constexpr int32_t MovementSpeed      = 0x78;  // @[HeroInfoData.MovementSpeed]
  static constexpr int32_t UnlockCost         = 0x7C;  // @[HeroInfoData.UnlockCost]
  static constexpr int32_t IsMeleeHero        = 0x80;  // @[HeroInfoData.IsMeleeHero]
  static constexpr int32_t IsAvailable        = 0x81;  // @[HeroInfoData.IsAvailable]
  static constexpr int32_t IsFirstAvailable   = 0x82;  // @[HeroInfoData.IsFirstAvailable]
};

struct Il2CppOffsets
{
  // HeroInfoData (static/base data) — confirmed
  static constexpr int32_t HeroInfoData_HeroKey     = 0x30;
  static constexpr int32_t HeroInfoData_HeroNameKey = 0x38;

  // vo base class (inherited into vh) — confirmed
  static constexpr int32_t Vo_StatContainer = 0x10;  // @[vo.StatContainer]

  // vh derived class — confirmed
  static constexpr int32_t Vh_HeroInfoDataRef = 0x30;  // @[vh.HeroInfoDataRef]
  static constexpr int32_t Vh_HeroBackRef     = 0x88;  // @[vh.HeroBackRef]

  // ze stat container - confirmed
  static constexpr int32_t Ze_StatsDictA = 0x18;  // @[ze.StatsDictA]
  static constexpr int32_t Ze_StatsDictB = 0x20;  // @[ze.StatsDictB]
};

// =========================================================================
// Save Data Offsets
// =========================================================================

// Save Manager (class bbb : ns<bbb>, TypeDefIndex: 1503)
// This is a singleton MonoBehaviour that holds the save data.
struct SaveManagerOffsets
{
  static constexpr int32_t AccountSaveData = 0x20;  // @[bbb.AccountSaveData]
  static constexpr int32_t PlayerSaveData  = 0x28;  // @[bbb.PlayerSaveData]
};

// PlayerSaveData (TypeDefIndex: 844)
struct PlayerSaveDataOffsets
{
  static constexpr int32_t CommonSaveData     = 0x10;  // @[PlayerSaveData.commonSaveData]
  static constexpr int32_t SettingSaveData    = 0x18;  // @[PlayerSaveData.settingSaveData]
  static constexpr int32_t BoxData            = 0x20;  // @[PlayerSaveData.BoxData]
  static constexpr int32_t CurrencySaveDatas  = 0x68;  // @[PlayerSaveData.currenySaveDatas]
  static constexpr int32_t HeroSaveDatas      = 0x70;  // @[PlayerSaveData.heroSaveDatas]
  static constexpr int32_t AttributeSaveDatas = 0x80;  // @[PlayerSaveData.attributeSaveDatas]
  static constexpr int32_t PetSaveData        = 0x88;  // @[PlayerSaveData.PetSaveData]
  static constexpr int32_t RuneSaveData       = 0x90;  // @[PlayerSaveData.RuneSaveData]
  static constexpr int32_t InventorySaveDatas = 0x98;  // @[PlayerSaveData.inventorySaveDatas]
  static constexpr int32_t CubeSaveLevelData  = 0xB8;  // @[PlayerSaveData.cubeSaveLevelData]
};

// HeroSaveData (TypeDefIndex: 1258)
struct HeroSaveDataOffsets
{
  static constexpr int32_t HeroKey   = 0x10;  // @[HeroSaveData.heroKey]
  static constexpr int32_t HeroLevel = 0x14;  // @[HeroSaveData.HeroLevel]
  static constexpr int32_t IsUnLock  = 0x18;  // @[HeroSaveData.IsUnLock]
  static constexpr int32_t HeroExp   = 0x20;  // @[HeroSaveData.HeroExp]
};

// CurrencySaveData (TypeDefIndex: 1257)
struct CurrencySaveDataOffsets
{
  static constexpr int32_t Key      = 0x10;  // @[CurrencySaveData.Key]
  static constexpr int32_t Quantity = 0x18;  // @[CurrencySaveData.Quantity]
};

// RuneSaveData (TypeDefIndex: 1266)
struct RuneSaveDataOffsets
{
  static constexpr int32_t RuneKey = 0x10;  // @[RuneSaveData.RuneKey]
  static constexpr int32_t Level   = 0x14;  // @[RuneSaveData.Level]
};

// CubeLevelSaveData
struct CubeLevelSaveDataOffsets
{
  static constexpr int32_t Level = 0x10;  // @[CubeSaveLevelData.Level]
  static constexpr int32_t Exp   = 0x14;  // @[CubeSaveLevelData.Exp]
};

// =========================================================================
// StageManager (TypeDefIndex: 816, extends ns<StageManager> singleton)
// =========================================================================
struct StageManagerOffsets
{
  static constexpr int32_t HeroList    = 0x30;   // @[StageManager.HeroList]
  static constexpr int32_t OnGetBox    = 0x100;  // @[StageManager.OnGetBox]
  static constexpr int32_t BoxDropDict = 0x140;  // @[StageManager.BoxDropDict]
};

// EBoxType values
struct EBoxType
{
  static constexpr int32_t NORMAL = 0;
  static constexpr int32_t BOSS   = 1;
};

// =========================================================================
// IL2CPP Generic Container Offsets
// =========================================================================

// Standard IL2CPP List<T> layout
struct Il2CppListOffsets
{
  static constexpr int32_t Items   = 0x10;  // T[] _items (backing array pointer)
  static constexpr int32_t Size    = 0x18;  // int _size
  static constexpr int32_t Version = 0x1C;  // int _version
};

// Standard IL2CPP Array header
struct Il2CppArrayOffsets
{
  static constexpr int32_t Length = 0x18;  // int length
  static constexpr int32_t Data   = 0x20;  // first element starts here
  // Each element is sizeof(T) for value types, or sizeof(pointer) for ref types
};

// Standard IL2CPP String header
struct Il2CppStringOffsets
{
  static constexpr int32_t Length = 0x10;  // int length
  static constexpr int32_t Chars  = 0x14;  // first char starts here
};

// Standard IL2CPP Dictionary<TKey, TValue> layout
struct Il2CppDictOffsets
{
  static constexpr int32_t Entries        = 0x18;  // Entry[] _entries
  static constexpr int32_t Count          = 0x20;  // int _count
  static constexpr int32_t ArrayLength    = 0x18;  // array header length
  static constexpr int32_t ArrayData      = 0x20;  // array data start
  static constexpr int32_t FreeCount      = 0x2C;  // int _freeCount
  static constexpr int32_t EntrySize      = 16;    // { int hashCode; int next; TKey; TValue }
  static constexpr int32_t EntryKeyOffset = 8;     // offset to key within entry
  static constexpr int32_t EntryValOffset = 12;    // offset to value within entry
};

// Global instance accessor
inline const Il2CppOffsets& CurrentOffsets()
{
  static Il2CppOffsets offsets;
  return offsets;
}
