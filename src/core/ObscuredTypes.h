#pragma once

#include "ProcessMemory.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <cstdio>

/* =========================================================================
 * ACTk Obscured Types — Encrypt / Decrypt / Read / Write
 *
 * Based on reverse-engineering of CodeStage Anti-Cheat Toolkit (ACTk)
 * from il2cppDumper output:
 *
 * ObscuredLong struct layout (from il2cpp.h):
 *   int32_t hash;            // offset +0x00  (4 bytes + 4 padding)
 *   int64_t hiddenValue;     // offset +0x08  (8 bytes)
 *   int64_t currentCryptoKey;// offset +0x10  (8 bytes)
 *   int64_t fakeValue;       // offset +0x18  (8 bytes)
 *   Total: 0x20 (32 bytes)
 *
 * ObscuredInt struct layout:
 *   int32_t hash;            // offset +0x00  (4 bytes)
 *   int32_t hiddenValue;     // offset +0x04  (4 bytes)
 *   int32_t currentCryptoKey;// offset +0x08  (4 bytes)
 *   int32_t fakeValue;       // offset +0x0C  (4 bytes)
 *   Total: 0x10 (16 bytes)
 *
 * ObscuredFloat struct layout:
 *   int32_t hash;            // offset +0x00  (4 bytes)
 *   int32_t hiddenValue;     // offset +0x04  (4 bytes)
 *   int32_t currentCryptoKey;// offset +0x08  (4 bytes)
 *   float   fakeValue;       // offset +0x0C  (4 bytes)
 *   ACTkByte4 hiddenValueOldByte4; // offset +0x10 (4 bytes)
 *   Total: 0x14 (20 bytes)
 *
 * ObscuredDouble struct layout:
 *   int32_t hash;            // offset +0x00  (4 bytes + 4 padding)
 *   int64_t hiddenValue;     // offset +0x08  (8 bytes)
 *   int64_t currentCryptoKey;// offset +0x10  (8 bytes)
 *   double  fakeValue;       // offset +0x18  (8 bytes)
 *   ACTkByte8 hiddenValueOldByte8; // offset +0x20 (8 bytes)
 *   Total: 0x28 (40 bytes)
 *
 * Encryption: hiddenValue = plainValue XOR currentCryptoKey
 *   - ObscuredLong.xzk(a, b)  = a ^ b     (RVA: 0x6D33F0)
 *   - ObscuredLong.xzl(a, b)  = a ^ b     (RVA: 0x6D3400)
 *   - ObscuredInt.xyr(a, b)   = a ^ b     (RVA: 0x6DA610)
 *   - ObscuredFloat uses int XOR via union
 *   - ObscuredDouble uses long XOR via union
 *
 * Hash validation:
 *   - For long:  hash = (int)(value ^ (value >> 32))  [.NET Int64.GetHashCode()]
 *   - For int:   hash = value                         [.NET Int32.GetHashCode()]
 *   - For float: hash = BitConverter.ToInt32(bytes)    [float -> int bits]
 *   - For double: hash = (int)(bits ^ (bits >> 32))   [double -> long bits -> hash]
 *
 *   xzr(long value, int hash) checks if hash matches expected.
 *   xzs(long value) generates and stores the hash.
 *
 * When ObscuredCheatingDetector is active, it calls xlj() which invokes
 * ysh<T>() to compare fakeValue with the decrypted value and the hash.
 * ========================================================================= */

namespace ObscuredOffsets {
  // ObscuredLong
  namespace Long {
    constexpr int Hash      = 0x00;
    constexpr int Hidden    = 0x08;
    constexpr int CryptoKey = 0x10;
    constexpr int FakeValue = 0x18;
    constexpr int Size      = 0x20;
  }  // namespace Long

  // ObscuredInt
  namespace Int {
    constexpr int Hash      = 0x00;
    constexpr int Hidden    = 0x04;
    constexpr int CryptoKey = 0x08;
    constexpr int FakeValue = 0x0C;
    constexpr int Size      = 0x10;
  }  // namespace Int

  // ObscuredFloat
  namespace Float {
    constexpr int Hash      = 0x00;
    constexpr int Hidden    = 0x04;
    constexpr int CryptoKey = 0x08;
    constexpr int FakeValue = 0x0C;
    constexpr int Size      = 0x14;
  }  // namespace Float

  // ObscuredDouble
  namespace Double {
    constexpr int Hash      = 0x00;
    constexpr int Hidden    = 0x08;
    constexpr int CryptoKey = 0x10;
    constexpr int FakeValue = 0x18;
    constexpr int Size      = 0x28;
  }  // namespace Double
}  // namespace ObscuredOffsets

/* -----------------------------------------------------------------------
 * Hash computation — mirrors .NET GetHashCode() for each type
 * ----------------------------------------------------------------------- */

// .NET Int64.GetHashCode(): return (int)(value ^ (value >> 32));
inline int32_t ComputeObscuredLongHash(int64_t value) { return (int32_t) (value ^ (value >> 32)); }

// .NET Int32.GetHashCode(): return value;
inline int32_t ComputeObscuredIntHash(int32_t value) { return value; }

// .NET Single.GetHashCode(): int bits = *(int*)&value; return bits;
// But if value == 0, returns 0 (handles -0.0f)
inline int32_t ComputeObscuredFloatHash(float value)
{
  if (value == 0.0f)
    return 0;
  int32_t bits;
  std::memcpy(&bits, &value, sizeof(int32_t));
  return bits;
}

// .NET Double.GetHashCode(): long bits = *(long*)&value; return (int)(bits ^ (bits >> 32));
// But if value == 0, returns 0 (handles -0.0)
inline int32_t ComputeObscuredDoubleHash(double value)
{
  if (value == 0.0)
    return 0;
  int64_t bits;
  std::memcpy(&bits, &value, sizeof(int64_t));
  return (int32_t) (bits ^ (bits >> 32));
}

/* -----------------------------------------------------------------------
 * Read helpers — decrypt an ObscuredType at a given memory address
 * ----------------------------------------------------------------------- */

struct ObscuredLongResult
{
  int32_t hash;
  int64_t hiddenValue;
  int64_t cryptoKey;
  int64_t fakeValue;
  int64_t decryptedValue;
  bool    hashValid;
};

struct ObscuredIntResult
{
  int32_t hash;
  int32_t hiddenValue;
  int32_t cryptoKey;
  int32_t fakeValue;
  int32_t decryptedValue;
  bool    hashValid;
};

struct ObscuredFloatResult
{
  int32_t hash;
  int32_t hiddenValue;
  int32_t cryptoKey;
  float   fakeValue;
  float   decryptedValue;
  bool    hashValid;
};

struct ObscuredDoubleResult
{
  int32_t hash;
  int64_t hiddenValue;
  int64_t cryptoKey;
  double  fakeValue;
  double  decryptedValue;
  bool    hashValid;
};

class ObscuredTypeHelper
{
public:
  explicit ObscuredTypeHelper(ProcessMemory& mem) :
      m_mem(mem)
  {
  }

  // =====================================================================
  // ObscuredLong
  // =====================================================================

  std::optional<ObscuredLongResult> ReadObscuredLong(uintptr_t addr) const
  {
    auto hash   = m_mem.ReadInt32(addr + ObscuredOffsets::Long::Hash);
    auto hidden = m_mem.ReadInt64(addr + ObscuredOffsets::Long::Hidden);
    auto key    = m_mem.ReadInt64(addr + ObscuredOffsets::Long::CryptoKey);
    auto fake   = m_mem.ReadInt64(addr + ObscuredOffsets::Long::FakeValue);

    if (!hash || !hidden || !key || !fake)
      return std::nullopt;

    ObscuredLongResult r;
    r.hash           = *hash;
    r.hiddenValue    = *hidden;
    r.cryptoKey      = *key;
    r.fakeValue      = *fake;
    r.decryptedValue = *hidden ^ *key;  // XOR decrypt
    r.hashValid      = (*hash == ComputeObscuredLongHash(r.decryptedValue));
    return r;
  }

  bool WriteObscuredLong(uintptr_t addr, int64_t newValue) const
  {
    // Read current crypto key
    auto key = m_mem.ReadInt64(addr + ObscuredOffsets::Long::CryptoKey);
    if (!key)
      return false;

    struct
    {
      int32_t hash;
      int32_t pad;
      int64_t hidden;
      int64_t key;
      int64_t fake;
    } data;

    data.hash   = ComputeObscuredLongHash(newValue);
    data.pad    = 0;
    data.hidden = newValue ^ *key;
    data.key    = *key;
    data.fake   = newValue;

    // Write all fields atomically to prevent AntiCheat race conditions
    return m_mem.WriteBytes(addr, &data, sizeof(data));
  }

  // =====================================================================
  // ObscuredInt
  // =====================================================================

  std::optional<ObscuredIntResult> ReadObscuredInt(uintptr_t addr) const
  {
    auto hash   = m_mem.ReadInt32(addr + ObscuredOffsets::Int::Hash);
    auto hidden = m_mem.ReadInt32(addr + ObscuredOffsets::Int::Hidden);
    auto key    = m_mem.ReadInt32(addr + ObscuredOffsets::Int::CryptoKey);
    auto fake   = m_mem.ReadInt32(addr + ObscuredOffsets::Int::FakeValue);

    if (!hash || !hidden || !key || !fake)
      return std::nullopt;

    ObscuredIntResult r;
    r.hash           = *hash;
    r.hiddenValue    = *hidden;
    r.cryptoKey      = *key;
    r.fakeValue      = *fake;
    r.decryptedValue = *hidden ^ *key;
    r.hashValid      = (*hash == ComputeObscuredIntHash(r.decryptedValue));
    return r;
  }

  bool WriteObscuredInt(uintptr_t addr, int32_t newValue) const
  {
    auto key = m_mem.ReadInt32(addr + ObscuredOffsets::Int::CryptoKey);
    if (!key)
      return false;

    struct
    {
      int32_t hash;
      int32_t hidden;
      int32_t key;
      int32_t fake;
    } data;

    data.hash   = ComputeObscuredIntHash(newValue);
    data.hidden = newValue ^ *key;
    data.key    = *key;
    data.fake   = newValue;

    return m_mem.WriteBytes(addr, &data, sizeof(data));
  }

  // =====================================================================
  // ObscuredFloat
  // =====================================================================

  std::optional<ObscuredFloatResult> ReadObscuredFloat(uintptr_t addr) const
  {
    auto hash   = m_mem.ReadInt32(addr + ObscuredOffsets::Float::Hash);
    auto hidden = m_mem.ReadInt32(addr + ObscuredOffsets::Float::Hidden);
    auto key    = m_mem.ReadInt32(addr + ObscuredOffsets::Float::CryptoKey);

    if (!hash || !hidden || !key)
      return std::nullopt;

    // Read fakeValue as float
    float fakeValue = 0.0f;
    auto  fakeBytes = m_mem.ReadInt32(addr + ObscuredOffsets::Float::FakeValue);
    if (fakeBytes)
      std::memcpy(&fakeValue, &*fakeBytes, sizeof(float));

    // Decrypt: XOR the int representations, then reinterpret as float
    int32_t decryptedBits = *hidden ^ *key;
    float   decrypted;
    std::memcpy(&decrypted, &decryptedBits, sizeof(float));

    ObscuredFloatResult r;
    r.hash           = *hash;
    r.hiddenValue    = *hidden;
    r.cryptoKey      = *key;
    r.fakeValue      = fakeValue;
    r.decryptedValue = decrypted;
    r.hashValid      = (*hash == ComputeObscuredFloatHash(decrypted));
    return r;
  }

  bool WriteObscuredFloat(uintptr_t addr, float newValue) const
  {
    auto key = m_mem.ReadInt32(addr + ObscuredOffsets::Float::CryptoKey);
    if (!key)
      return false;

    int32_t newValueBits;
    std::memcpy(&newValueBits, &newValue, sizeof(int32_t));
    int32_t newHidden = newValueBits ^ *key;
    int32_t newHash   = ComputeObscuredFloatHash(newValue);

    // Write fakeValue as float (raw int32 copy)
    int32_t fakeAsInt;
    std::memcpy(&fakeAsInt, &newValue, sizeof(int32_t));

    bool ok = true;
    ok &= m_mem.WriteInt32(addr + ObscuredOffsets::Float::Hidden, newHidden);
    ok &= m_mem.WriteInt32(addr + ObscuredOffsets::Float::FakeValue, fakeAsInt);
    ok &= m_mem.WriteInt32(addr + ObscuredOffsets::Float::Hash, newHash);
    return ok;
  }

  // =====================================================================
  // ObscuredDouble
  // =====================================================================

  std::optional<ObscuredDoubleResult> ReadObscuredDouble(uintptr_t addr) const
  {
    auto hash   = m_mem.ReadInt32(addr + ObscuredOffsets::Double::Hash);
    auto hidden = m_mem.ReadInt64(addr + ObscuredOffsets::Double::Hidden);
    auto key    = m_mem.ReadInt64(addr + ObscuredOffsets::Double::CryptoKey);

    if (!hash || !hidden || !key)
      return std::nullopt;

    // Read fakeValue as double
    double fakeValue = 0.0;
    auto   fakeBytes = m_mem.ReadInt64(addr + ObscuredOffsets::Double::FakeValue);
    if (fakeBytes) {
      int64_t fb = *fakeBytes;
      std::memcpy(&fakeValue, &fb, sizeof(double));
    }

    // Decrypt: XOR the int64 representations, then reinterpret as double
    int64_t decryptedBits = *hidden ^ *key;
    double  decrypted;
    std::memcpy(&decrypted, &decryptedBits, sizeof(double));

    ObscuredDoubleResult r;
    r.hash           = *hash;
    r.hiddenValue    = *hidden;
    r.cryptoKey      = *key;
    r.fakeValue      = fakeValue;
    r.decryptedValue = decrypted;
    r.hashValid      = (*hash == ComputeObscuredDoubleHash(decrypted));
    return r;
  }

  bool WriteObscuredDouble(uintptr_t addr, double newValue) const
  {
    auto key = m_mem.ReadInt64(addr + ObscuredOffsets::Double::CryptoKey);
    if (!key)
      return false;

    int64_t newValueBits;
    std::memcpy(&newValueBits, &newValue, sizeof(int64_t));
    int64_t newHidden = newValueBits ^ *key;
    int32_t newHash   = ComputeObscuredDoubleHash(newValue);

    // Write fakeValue as raw bits
    bool ok = true;
    ok &= m_mem.WriteInt64(addr + ObscuredOffsets::Double::Hidden, newHidden);
    ok &= m_mem.WriteInt64(addr + ObscuredOffsets::Double::FakeValue, newValueBits);
    ok &= m_mem.WriteInt32(addr + ObscuredOffsets::Double::Hash, newHash);
    return ok;
  }

  // =====================================================================
  // Debug: Dump an ObscuredLong to stdout
  // =====================================================================

  void DumpObscuredLong(uintptr_t addr, const char* label = "ObscuredLong") const
  {
    auto r = ReadObscuredLong(addr);
    if (!r) {
      printf("[%s] Failed to read at 0x%llX\n", label, (unsigned long long) addr);
      return;
    }
    printf(
      "[%s @ 0x%llX] hash=%08X hidden=%lld key=%lld fake=%lld decrypted=%lld hashValid=%s\n", label,
      (unsigned long long) addr, r->hash, (long long) r->hiddenValue, (long long) r->cryptoKey,
      (long long) r->fakeValue, (long long) r->decryptedValue, r->hashValid ? "YES" : "NO"
    );
  }

  void DumpObscuredInt(uintptr_t addr, const char* label = "ObscuredInt") const
  {
    auto r = ReadObscuredInt(addr);
    if (!r) {
      printf("[%s] Failed to read at 0x%llX\n", label, (unsigned long long) addr);
      return;
    }
    printf(
      "[%s @ 0x%llX] hash=%08X hidden=%d key=%d fake=%d decrypted=%d hashValid=%s\n", label,
      (unsigned long long) addr, r->hash, r->hiddenValue, r->cryptoKey, r->fakeValue,
      r->decryptedValue, r->hashValid ? "YES" : "NO"
    );
  }

  void DumpObscuredDouble(uintptr_t addr, const char* label = "ObscuredDouble") const
  {
    auto r = ReadObscuredDouble(addr);
    if (!r) {
      printf("[%s] Failed to read at 0x%llX\n", label, (unsigned long long) addr);
      return;
    }
    printf(
      "[%s @ 0x%llX] hash=%08X hidden=%lld key=%lld fake=%.2f decrypted=%.2f hashValid=%s\n", label,
      (unsigned long long) addr, r->hash, (long long) r->hiddenValue, (long long) r->cryptoKey,
      r->fakeValue, r->decryptedValue, r->hashValid ? "YES" : "NO"
    );
  }

private:
  ProcessMemory& m_mem;
};
