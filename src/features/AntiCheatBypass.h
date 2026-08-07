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
      {"InjectionDetector.yri", 0x6E9B70},  // @RVA[InjectionDetector.yri]  // internal override detect
      {"InjectionDetector.yrn", 0x6E9CF0},  // @RVA[InjectionDetector.yrn]  // static start

      // SpeedHackDetector
      {"SpeedHackDetector.yso", 0x6EFAF0},  // @RVA[SpeedHackDetector.yso]  // static start
      {"SpeedHackDetector.ysp", 0x6EFB20},  // @RVA[SpeedHackDetector.ysp]
      {"SpeedHackDetector.ysq", 0x6EFD90},  // @RVA[SpeedHackDetector.ysq]
      {"SpeedHackDetector.ysr", 0x6EFEB0},  // @RVA[SpeedHackDetector.ysr]
      {"SpeedHackDetector.yss", 0x6EFFA0},  // @RVA[SpeedHackDetector.yss]
      {"SpeedHackDetector.yst", 0x6F0060},  // @RVA[SpeedHackDetector.yst]
      {"SpeedHackDetector.ysu", 0x6F00F0},  // @RVA[SpeedHackDetector.ysu]
      {"SpeedHackDetector.ysv", 0x6F0200},  // @RVA[SpeedHackDetector.ysv]
      {"SpeedHackDetector.ysw", 0x6F0310},  // @RVA[SpeedHackDetector.ysw]
      {"SpeedHackDetector.ysx", 0x6F0470},  // @RVA[SpeedHackDetector.ysx]
      {"SpeedHackDetector.ysy", 0x6F05B0},  // @RVA[SpeedHackDetector.ysy]
      {"SpeedHackDetector.ysz", 0x6F06E0},  // @RVA[SpeedHackDetector.ysz]
      {"SpeedHackDetector.yta", 0x6F0800},  // @RVA[SpeedHackDetector.yta]
      {"SpeedHackDetector.ytb", 0x6F08F0},  // @RVA[SpeedHackDetector.ytb]
      {"SpeedHackDetector.ytc", 0x6F0B60},  // @RVA[SpeedHackDetector.ytc]
      {"SpeedHackDetector.ytd", 0x6F0C20},  // @RVA[SpeedHackDetector.ytd]
      {"SpeedHackDetector.yte", 0x6F0C70},  // @RVA[SpeedHackDetector.yte]
      {"SpeedHackDetector.ytf", 0x6F0D10},  // @RVA[SpeedHackDetector.ytf]
      {"SpeedHackDetector.Update", 0x6EF560},  // @RVA[SpeedHackDetector.Update]
      {"SpeedHackDetector.yri", 0x6EFA40},  // @RVA[SpeedHackDetector.yri]
      {"SpeedHackDetector.yre", 0x6EF9E0},  // @RVA[SpeedHackDetector.yre]

      // TimeCheatingDetector
      {"TimeCheatingDetector.Update", 0x6EF9A0},  // @RVA[TimeCheatingDetector.Update]
      {"TimeCheatingDetector.yuw", 0x6F0180},  // @RVA[TimeCheatingDetector.yuw]
      {"TimeCheatingDetector.yux", 0x6F01B0},  // @RVA[TimeCheatingDetector.yux]
      {"TimeCheatingDetector.yuz", 0x6F0400},  // @RVA[TimeCheatingDetector.yuz]
      {"TimeCheatingDetector.yvs", 0x6F2E60},  // @RVA[TimeCheatingDetector.yvs]
      {"TimeCheatingDetector.yvy", 0x6F33D0},  // @RVA[TimeCheatingDetector.yvy]

      // ObscuredCheatingDetector
      {"ObscuredCheatingDetector.yri", 0x6E9F60},  // @RVA[ObscuredCheatingDetector.yri]  // internal override detect
      {"ObscuredCheatingDetector.yrx", 0x6EA020},  // @RVA[ObscuredCheatingDetector.yrx]
      {"ObscuredCheatingDetector.yry", 0x6EA170},  // @RVA[ObscuredCheatingDetector.yry]
      {"ObscuredCheatingDetector.yrz", 0x6EA1C0},  // @RVA[ObscuredCheatingDetector.yrz]  // static start method
      {"ObscuredCheatingDetector.ysa", 0x6EA2D0},  // @RVA[ObscuredCheatingDetector.ysa]  // stop?
      {"ObscuredCheatingDetector.ysb", 0x6EA3E0},  // @RVA[ObscuredCheatingDetector.ysb]
      {"ObscuredCheatingDetector.ysd", 0x6EA650},  // @RVA[ObscuredCheatingDetector.ysd]  // some internal detect logic
      {"ObscuredCheatingDetector.yse", 0x6EA7E0},  // @RVA[ObscuredCheatingDetector.yse]  // obsolete
      {"ObscuredCheatingDetector.ysf", 0x6EA8C0},  // @RVA[ObscuredCheatingDetector.ysf]  // more logic

      // WallHackDetector
      {"WallHackDetector.Update", 0x6F7A20},  // @RVA[WallHackDetector.Update]
      {"WallHackDetector.FixedUpdate", 0x6F6DD0},  // @RVA[WallHackDetector.FixedUpdate]
      {"WallHackDetector.yxf", 0x6F8310},  // @RVA[WallHackDetector.yxf]
      {"WallHackDetector.yxg", 0x6F8340},  // @RVA[WallHackDetector.yxg]
      {"WallHackDetector.yxh", 0x6F8570},  // @RVA[WallHackDetector.yxh]
      {"WallHackDetector.yri", 0x6F7ED0},  // @RVA[WallHackDetector.yri]
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
