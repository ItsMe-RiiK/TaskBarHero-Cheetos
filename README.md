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
3. The executable will be generated at `./build/TBH-GodMode.exe` along with the launcher scripts (`launch.bat`, `launch_linux.sh`, `launch_macos.sh`).

---

## Usage

You must run the tool while the **TaskBarHero** game is open and running.

**We provide wrapper scripts to make launching as easy as possible!** If you compiled from source, the scripts are in your `build/` folder. If you downloaded a Release `.zip`, the scripts are right next to the executable.

### Windows
Double-click `launch.bat` or run it from your command prompt. **Note:** You may need to run it as Administrator so it has permission to write to the game's memory.
```powershell
.\launch.bat
```

### Linux (Steam Proton / Wine)
If you are playing the game on Linux via Steam (Proton), our script automatically handles `protontricks` for you.
```bash
./launch_linux.sh
```

### macOS (Wine / Whisky / CrossOver)
MacOS cannot run Windows games natively. If you are playing the game through a translation layer like Wine, Whisky, or CrossOver, our script will launch the tool inside that same environment.
```bash
./launch_macos.sh
```


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

## LICENSE
This Project under [MIT LICENSE](License)