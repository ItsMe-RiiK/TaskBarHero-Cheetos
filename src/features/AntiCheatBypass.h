#pragma once
#include "../core/ProcessMemory.h"

#include <cstdint>
#include <string>

/* -----------------------------------------------------------------------
 * Anti-Cheat Bypass — disables CodeStage ACTk detectors.
 *
 * The game uses ACTk (Anti-Cheat Toolkit) with these detectors:
 *   - SpeedHackDetector
 *   - InjectionDetector
 *   - ObscuredCheatingDetector
 *   - TimeCheatingDetector
 *   - WallHackDetector
 *
 * All detectors inherit from ACTkDetectorBase<T> which inherits from
 * dfh<T> (Unity MonoBehaviour singleton).
 *
 *   1. Find GameAssembly.dll in memory
 *   2. Locate detector Start/Update methods via known RVAs
 *   3. Patch the first byte to RET (0xC3) to disable them
 *
 * RVAs from dump.cs (confirmed):
 *   InjectionDetector:
 *     .ctor:     RVA 0x6E9B30
 *     yri():     RVA 0x6E9B70  (internal override, likely the detect method)
 *     yrn():     RVA 0x6E9CF0  (static start method)
 *
 * Note: RVAs are relative to GameAssembly.dll base address.
 * ----------------------------------------------------------------------- */
class AntiCheatBypass
{
public:
  explicit AntiCheatBypass(ProcessMemory& mem) :
      m_mem(mem)
  {
  }

  bool IsActive() const { return m_active; }

  void Toggle() { m_active = !m_active; }
  void Enable() { m_active = true; }
  void Disable() { m_active = false; }

  // Apply the bypass by patching detector methods
  std::string Apply()
  {
    if (!m_active)
      return "";

    // Find GameAssembly.dll
    auto modInfo = m_mem.FindModule(L"GameAssembly.dll");
    if (!modInfo)
      return "[AntiCheat] GameAssembly.dll not found. Is the game running?";

    uintptr_t gaBase   = modInfo->base;
    m_gameAssemblyBase = gaBase;

    int         patchCount = 0;
    std::string details;

    // Patch each detector's key method(s) with RET (0xC3)
    // This prevents the detector from executing its detection logic.
    struct PatchTarget
    {
      const char* name;
      uintptr_t   rva;
    };

    static const PatchTarget targets[] = {
      // InjectionDetector
      {"InjectionDetector.yri", 0x6E9B70},  // internal override detect
      {"InjectionDetector.yrn", 0x6E9CF0},  // static start

      // SpeedHackDetector
      {"SpeedHackDetector.yso", 0x6EFAF0},  // static start
      {"SpeedHackDetector.ysp", 0x6EFB20},
      {"SpeedHackDetector.ysq", 0x6EFD90},
      {"SpeedHackDetector.ysr", 0x6EFEB0},
      {"SpeedHackDetector.yss", 0x6EFFA0},
      {"SpeedHackDetector.yst", 0x6F0060},
      {"SpeedHackDetector.ysu", 0x6F00F0},
      {"SpeedHackDetector.ysv", 0x6F0200},
      {"SpeedHackDetector.ysw", 0x6F0310},
      {"SpeedHackDetector.ysx", 0x6F0470},
      {"SpeedHackDetector.ysy", 0x6F05B0},
      {"SpeedHackDetector.ysz", 0x6F06E0},
      {"SpeedHackDetector.yta", 0x6F0800},
      {"SpeedHackDetector.ytb", 0x6F08F0},
      {"SpeedHackDetector.ytc", 0x6F0B60},
      {"SpeedHackDetector.ytd", 0x6F0C20},
      {"SpeedHackDetector.yte", 0x6F0C70},
      {"SpeedHackDetector.ytf", 0x6F0D10},
      {"SpeedHackDetector.Update", 0x6EF560},
      {"SpeedHackDetector.yri", 0x6EFA40},
      {"SpeedHackDetector.yre", 0x6EF9E0},

      // TimeCheatingDetector
      {"TimeCheatingDetector.Update", 0x6EF9A0},
      {"TimeCheatingDetector.yuw", 0x6F0180},
      {"TimeCheatingDetector.yux", 0x6F01B0},
      {"TimeCheatingDetector.yuz", 0x6F0400},
      {"TimeCheatingDetector.yvs", 0x6F2E60},
      {"TimeCheatingDetector.yvy", 0x6F33D0},

      // ObscuredCheatingDetector
      {"ObscuredCheatingDetector.yri", 0x6E9F60},  // internal override detect
      {"ObscuredCheatingDetector.yrx", 0x6EA020},
      {"ObscuredCheatingDetector.yry", 0x6EA170},
      {"ObscuredCheatingDetector.yrz", 0x6EA1C0},  // static start method
      {"ObscuredCheatingDetector.ysa", 0x6EA2D0},  // stop?
      {"ObscuredCheatingDetector.ysb", 0x6EA3E0},
      {"ObscuredCheatingDetector.ysd", 0x6EA650},  // some internal detect logic
      {"ObscuredCheatingDetector.yse", 0x6EA7E0},  // obsolete
      {"ObscuredCheatingDetector.ysf", 0x6EA8C0},  // more logic

      // WallHackDetector
      {"WallHackDetector.Update", 0x6F7A20},
      {"WallHackDetector.FixedUpdate", 0x6F6DD0},
      {"WallHackDetector.yxf", 0x6F8310},
      {"WallHackDetector.yxg", 0x6F8340},
      {"WallHackDetector.yxh", 0x6F8570},
      {"WallHackDetector.yri", 0x6F7ED0},
    };

    for (const auto& t : targets) {
      uintptr_t addr = gaBase + t.rva;

      // Read original bytes first (for potential restore)
      m_mem.ReadBytes(addr, &m_origBytes[patchCount * 3], 3);

      // Write XOR EAX, EAX; RET instruction (31 C0 C3)
      uint8_t retInstr[3] = {0x31, 0xC0, 0xC3};
      DWORD   oldProtect  = 0;

      // Need to change memory protection first since code pages are typically RX
      if (VirtualProtectEx(m_mem.Handle(), (LPVOID) addr, 3, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        if (m_mem.WriteBytes(addr, retInstr, 3)) {
          patchCount++;
          details += "  Patched " + std::string(t.name) + " @ 0x" + ToHex(addr) + "\n";
        }
        // Restore original protection
        VirtualProtectEx(m_mem.Handle(), (LPVOID) addr, 3, oldProtect, &oldProtect);
      }
    }

    if (patchCount > 0)
      return "[AntiCheat] Bypassed " + std::to_string(patchCount) + " detector(s).\n" + details;
    else
      return "[AntiCheat] No detectors patched (may need RVA update).";
  }

private:
  static std::string ToHex(uintptr_t v)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llX", (unsigned long long) v);
    return buf;
  }

  ProcessMemory& m_mem;
  bool           m_active           = true;
  uintptr_t      m_gameAssemblyBase = 0;
  uint8_t        m_origBytes[256]   = {};  // saved original bytes for restore
};
