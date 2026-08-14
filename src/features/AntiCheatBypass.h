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
      // Direct Obscured Types Bypass (Bypass op_Implicit completely)
      {"ObscuredInt.op_Implicit", 0x72D370},     // @RVA[ObscuredInt.op_Implicit] [UNCHANGED]
      {"ObscuredFloat.op_Implicit", 0x72B0A0},   // @RVA[ObscuredFloat.op_Implicit] [UNCHANGED]
      {"ObscuredDouble.op_Implicit", 0x72A2D0},  // @RVA[ObscuredDouble.op_Implicit] [UNCHANGED]
      {"ObscuredLong.op_Implicit", 0x72DC70},    // @RVA[ObscuredLong.op_Implicit]
                                                 // @ACTK_AUTOGEN_START
      {"InjectionDetector.yum", 0x73CC70},
      {"InjectionDetector.yun", 0x73CCF0},
      {"InjectionDetector.yuo", 0x73CD70},
      {"InjectionDetector.yup", 0x73CDF0},
      {"InjectionDetector.yuh", 0x73CAF0},
      {"ObscuredCheatingDetector.yuy", 0x73D140},
      {"ObscuredCheatingDetector.yuz", 0x73D250},
      {"ObscuredCheatingDetector.yvc", 0x73D5D0},
      {"ObscuredCheatingDetector.yvd", 0x73D760},
      {"ObscuredCheatingDetector.yve", 0x73D840},
      {"ObscuredCheatingDetector.yuh", 0x73CEE0},
      {"SpeedHackDetector.yvt", 0x743070},
      {"SpeedHackDetector.yvu", 0x743180},
      {"SpeedHackDetector.yvv", 0x743290},
      {"SpeedHackDetector.yvw", 0x7433F0},
      {"SpeedHackDetector.yvx", 0x743530},
      {"SpeedHackDetector.yvy", 0x743660},
      {"SpeedHackDetector.yvz", 0x743780},
      {"SpeedHackDetector.Update", 0x7424E0},
      {"SpeedHackDetector.yuh", 0x7429C0},
      {"SpeedHackDetector.yud", 0x742960},
      {"SpeedHackDetector.ywc", 0x743BA0},
      {"SpeedHackDetector.ywe", 0x743C90},
      {"SpeedHackDetector.ywf", 0x743CD0},
      {"SpeedHackDetector.ywg", 0x743CF0},
      {"TimeCheatingDetector.ywq", 0x6B1620},
      {"TimeCheatingDetector.yws", 0x758A80},
      {"TimeCheatingDetector.ywu", 0x6B1620},
      {"TimeCheatingDetector.yww", 0x758BD0},
      {"TimeCheatingDetector.ywy", 0x6B1620},
      {"TimeCheatingDetector.yxa", 0x758CC0},
      {"TimeCheatingDetector.yxf", 0x759120},
      {"TimeCheatingDetector.yxc", 0x759040},
      {"TimeCheatingDetector.yxd", 0x7590D0},
      {"TimeCheatingDetector.yxj", 0x744430},
      {"TimeCheatingDetector.yxk", 0x7444D0},
      {"TimeCheatingDetector.yxm", 0x744580},
      {"TimeCheatingDetector.yxo", 0x744650},
      {"TimeCheatingDetector.yxq", 0x744670},
      {"TimeCheatingDetector.yxs", 0x7446B0},
      {"TimeCheatingDetector.yxu", 0x7446F0},
      {"TimeCheatingDetector.Update", 0x743F20},
      {"TimeCheatingDetector.yxy", 0x744980},
      {"TimeCheatingDetector.yxz", 0x744A90},
      {"TimeCheatingDetector.yym", 0x745590},
      {"TimeCheatingDetector.yyp", 0x745CA0},
      {"TimeCheatingDetector.yuh", 0x744350},
      {"TimeCheatingDetector.yue", 0x744300},
      {"TimeCheatingDetector.yud", 0x744290},
      {"TimeCheatingDetector.yyy", 0x7463C0},
      {"TimeCheatingDetector.yyz", 0x746500},
      {"TimeCheatingDetector.yza", 0x746590},
      {"TimeCheatingDetector.yzb", 0x746650},
      {"TimeCheatingDetector.yzc", 0x746710},
      {"TimeCheatingDetector.yzd", 0x7467C0},
      {"TimeCheatingDetector.yzf", 0x6B1620},
      {"TimeCheatingDetector.yzg", 0x6B1620},
      {"TimeCheatingDetector.yzh", 0x6B1620},
      {"TimeCheatingDetector.yzi", 0x6B1620},
      {"WallHackDetector.yzn", 0x6B1620},
      {"WallHackDetector.yzp", 0x76C9E0},
      {"WallHackDetector.yzr", 0x6B1620},
      {"WallHackDetector.yzt", 0x76CAE0},
      {"WallHackDetector.yzx", 0x74AF20},
      {"WallHackDetector.yzz", 0x74B000},
      {"WallHackDetector.zab", 0x74B0E0},
      {"WallHackDetector.zad", 0x74B1C0},
      {"WallHackDetector.zaj", 0x74B740},
      {"WallHackDetector.zak", 0x74B850},
      {"WallHackDetector.zal", 0x74B960},
      {"WallHackDetector.FixedUpdate", 0x749D50},
      {"WallHackDetector.Update", 0x74A9A0},
      {"WallHackDetector.yuh", 0x74AE50},
      {"WallHackDetector.yue", 0x74ACC0},
      {"WallHackDetector.yud", 0x74AC50},
      {"WallHackDetector.zan", 0x74BCE0},
      {"WallHackDetector.zap", 0x74D3E0},
      {"WallHackDetector.zas", 0x74D610},
      {"WallHackDetector.zat", 0x74D6D0},
      {"WallHackDetector.zau", 0x74D7C0},
      {"WallHackDetector.zav", 0x74D860},
      {"WallHackDetector.zaw", 0x74D8F0},
      {"WallHackDetector.zax", 0x74D950},
      {"WallHackDetector.zay", 0x74D9B0},
      {"WallHackDetector.zaz", 0x74DB50},
      {"WallHackDetector.zba", 0x74DD20},
      {"WallHackDetector.zbb", 0x74DDE0},
      // @ACTK_AUTOGEN_END
    };

    for (const auto& t : targets) {
      uintptr_t addr = gaBase + t.rva;

      // Skip missing RVAs
      if (t.rva == 0)
        continue;

      // Check if it's a direct bypass for Obscured types
      bool isDirectBypass = (std::string(t.name).find("op_Implicit") != std::string::npos);

      // Write instructions
      uint8_t retInstr[16];
      size_t  instrSize = 3;

      if (isDirectBypass) {
        if (std::string(t.name).find("ObscuredFloat") != std::string::npos) {
          // struct ObscuredFloat { int hash (0x0), int hiddenValue (0x4), int currentCryptoKey (0x8) }
          // mov eax, dword ptr [rcx + 4] (8B 41 04)
          // xor eax, dword ptr [rcx + 8] (33 41 08)
          // movd xmm0, eax (66 0F 6E C0)
          // ret (C3)
          // Completely bypasses hash checks and returns decrypted float in xmm0
          uint8_t bypassInstr[12] = {0x8B, 0x41, 0x04, 0x33, 0x41, 0x08,
                                     0x66, 0x0F, 0x6E, 0xC0, 0xC3};
          memcpy(retInstr, bypassInstr, 11);
          instrSize = 11;
        }
        else if (std::string(t.name).find("ObscuredDouble") != std::string::npos) {
          // struct ObscuredDouble { int hash (0x0), long hiddenValue (0x8), long currentCryptoKey (0x10) }
          // mov rax, qword ptr [rcx + 8]  (48 8B 41 08)
          // xor rax, qword ptr [rcx + 10] (48 33 41 10)
          // movq xmm0, rax                (66 48 0F 6E C0)
          // ret (C3)
          uint8_t bypassInstr[14] = {0x48, 0x8B, 0x41, 0x08, 0x48, 0x33, 0x41,
                                     0x10, 0x66, 0x48, 0x0F, 0x6E, 0xC0, 0xC3};
          memcpy(retInstr, bypassInstr, 14);
          instrSize = 14;
        }
        else if (std::string(t.name).find("ObscuredLong") != std::string::npos) {
          // struct ObscuredLong { int hash (0x0), long hiddenValue (0x8), long currentCryptoKey (0x10) }
          // mov rax, qword ptr [rcx + 8]  (48 8B 41 08)
          // xor rax, qword ptr [rcx + 10] (48 33 41 10)
          // ret (C3)
          // Completely bypasses hash checks and returns decrypted long in rax
          uint8_t bypassInstr[9] = {0x48, 0x8B, 0x41, 0x08, 0x48, 0x33, 0x41, 0x10, 0xC3};
          memcpy(retInstr, bypassInstr, 9);
          instrSize = 9;
        }
        else {
          // struct ObscuredInt { int hash (0x0), int hiddenValue (0x4), int currentCryptoKey (0x8) }
          // mov eax, dword ptr [rcx + 4] (8B 41 04)
          // xor eax, dword ptr [rcx + 8] (33 41 08)
          // ret (C3)
          // Completely bypasses hash checks and returns decrypted int in eax
          uint8_t bypassInstr[7] = {0x8B, 0x41, 0x04, 0x33, 0x41, 0x08, 0xC3};
          memcpy(retInstr, bypassInstr, 7);
          instrSize = 7;
        }
      }
      else if (std::string(t.name).find("ObscuredLong") != std::string::npos) {
        // struct ObscuredLong { int hash (0x0), long hiddenValue (0x8), long currentCryptoKey (0x10) }
        // mov rax, qword ptr [rcx + 8]  (48 8B 41 08)
        // xor rax, qword ptr [rcx + 10] (48 33 41 10)
        // ret (C3)
        // Completely bypasses hash checks and returns decrypted long in rax
        uint8_t bypassInstr[9] = {0x48, 0x8B, 0x41, 0x08, 0x48, 0x33, 0x41, 0x10, 0xC3};
        memcpy(retInstr, bypassInstr, 9);
        instrSize = 9;
      }
      else {
        // XOR EAX, EAX; RET instruction (31 C0 C3)
        uint8_t nullRet[3] = {0x31, 0xC0, 0xC3};
        memcpy(retInstr, nullRet, 3);
      }

      DWORD oldProtect = 0;

      // Need to change memory protection first since code pages are typically RX
      if (
        VirtualProtectEx(
          m_mem.Handle(), (LPVOID) addr, instrSize, PAGE_EXECUTE_READWRITE, &oldProtect
        )
      ) {
        if (m_mem.WriteBytes(addr, retInstr, instrSize)) {
          patchCount++;
          details += "  Patched " + std::string(t.name) + " @ 0x" + ToHex(addr) + "\n";
        }
        // Restore original protection
        VirtualProtectEx(m_mem.Handle(), (LPVOID) addr, instrSize, oldProtect, &oldProtect);
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
