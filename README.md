# TaskBarHero - GodMode

A lightweight, lightning-fast C++ memory scanner and trainer for **TaskBarHero**. This tool bypasses the need for Cheat Engine or Lua scripts by directly interacting with the game's memory (using the Win32 API) to find IL2CPP objects and inject "God Mode" stats in real-time.

## Features
- **No GUI Required:** Runs directly from the terminal.
- **IL2CPP Exact Pointer Chaining:** Follows the exact object pointers instead of blindly guessing memory boundaries.
- **Instant Injection:** Continuously injects max stats (Max HP, Attack Speed, Crit, Armor, Movement Speed) into the live game memory.
- **Cross-Platform Compatibility:** Can be compiled and run on Windows natively, or on Linux/macOS via Wine/Proton.

---

## Prerequisites
To compile the source code, you need:
- [CMake](https://cmake.org/) (v3.10 or higher)
- **Windows:** Visual Studio (MSVC) or MinGW
- **Linux/macOS:** MinGW-w64 (for cross-compiling to Windows `.exe`)

---

## How to Build

We use CMake to build the project. To ensure the memory scanner runs at maximum speed, **always build in Release mode**.

1. Clone the repository and navigate to the folder.
2. Generate the build files and compile the executable:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
3. The executable will be generated at `./build/HeroStatsFinder.exe`.

---

## Usage

You must run the tool while the **TaskBarHero** game is open and running.

### Windows
Run the executable directly from your command prompt or PowerShell. **Note:** You may need to run it as Administrator so it has permission to write to the game's memory.
```powershell
cd build
.\HeroStatsFinder.exe
```

### Linux (Steam Proton / Wine)
If you are playing the game on Linux via Steam (Proton), you need to run the tool inside the exact same Proton container/prefix as the game so they share the same memory space. You can use `protontricks` (replace `3678970` with the actual Steam App ID if different).
```bash
protontricks -c "wine ./build/HeroStatsFinder.exe" 3678970
```

### macOS (Wine / Whisky / CrossOver)
MacOS cannot run Windows games natively. If you are playing the game through a translation layer like Wine, Whisky, or CrossOver, you must run the tool inside that same environment.
```bash
wine ./build/HeroStatsFinder.exe
```

---

## Customizing Cheat Values

If you want to modify which stats are injected or change the values (e.g., enable DPS modification), open `src/main.cpp` and locate the `targetStats` vector:

```cpp
std::vector<CheatTarget> targetStats = {
  {"MHP", StatType::MaxHp, 999999999.0f},
  // Uncomment and change the value below to modify DPS
  // {"DPS", StatType::AttackDamage, 999999999.0f}, 
  {"ATK_SPD", StatType::AttackSpeed, 999999999.0f},
  // ...
};
```
After making changes, remember to re-run `cmake --build build` to compile the new values into the executable!