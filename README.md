<div align="center">
  
# TaskBarHero - Cheetos

[![Build Status](https://github.com/ItsMe-RiiK/TaskBarHero-Cheetos/actions/workflows/build.yml/badge.svg)](https://github.com/ItsMe-RiiK/TaskBarHero-Cheetos/actions)
[![Cheetos Version](https://img.shields.io/badge/version-2.0.0-blue)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-green)](#)
[![Game Version](https://img.shields.io/badge/Tested%20on-latest%20game%20version-orange)](#)
[![Latest Version](https://img.shields.io/badge/latest-1.01.04.-blue)](#)

A feature-rich, interactive graphical memory tool for **TaskBarHero**. 

</div>

---

## Overview

**TaskBarHero - Cheetos** directly interacts with the game's memory using the Win32 API and IL2CPP metadata to provide a comprehensive suite of enhancements. Everything is controlled from a single lightweight, blazing-fast GUI built with **Dear ImGui**.

> **Note:** This project has been tested and verified on game version **1.01.04**. The project will be updated if changes to the game affect the memory offsets.

---

## Features

| Feature | Description | Status | Type |
|---------|-------------|--------|-------|
| **Bypass Anti-Cheat** | Neutralizes anticheat detector methods when attached. | ✅ Works | Automatic |
| **God Mode** | One-shot injection of max HP, Attack Speed, Crit, Armor, and Movement Speed into all active heroes. | ✅ Works | Repeatable |
| **Rune Unlocker** | Safely unlock and max-level all runes for free. | ✅ Works | Interactive |
| **Speedhack** | Increases the global speed of the game. | ✅ Works | Toggle |
| **EXP Multiplier** | Multiplies the EXP earned from killing monsters. | ✅ Works | Interactive |

### Technical Capabilities
- **Seamless GUI:** A beautifully rendered floating window (via OpenGL3/GLFW) for real-time interaction.
- **Hot-Pluggable:** Connect and disconnect from the game instantly without restarting the tool.
- **Cross-Platform:** Native Windows `.exe` that runs flawlessly on Linux and macOS via Proton/Wine.
- **Automated Offset Updating:** Ships with Python scripting to effortlessly upgrade IL2CPP offsets after game patches.

---

## Getting Started

### Option A: Download Pre-compiled Release (Recommended)
1. Navigate to the **[Releases](../../releases)** page.
2. Download the latest `TBH-Cheetos-Release.zip`.
3. Extract the ZIP file to a folder on your computer.

### Option B: Build from Source
<details>
<summary>Click here for build instructions</summary>

#### Prerequisites
- [CMake](https://cmake.org/) (v3.10 or higher)
- [vcpkg](https://github.com/microsoft/vcpkg) (dependencies are managed automatically)
- **Windows:** Visual Studio (MSVC) or MinGW-w64
- **Linux/macOS:** MinGW-w64 (for cross-compiling to Windows `.exe`)

#### 1. Clone the repository
```bash
git clone --recursive https://github.com/ItsMe-RiiK/TaskBarHero-Cheetos.git
cd TaskBarHero-Cheetos
```

#### 2. Windows (MSVC)
```powershell
.\vcpkg\bootstrap-vcpkg.bat
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

#### 3. Linux / macOS (Cross-compiling via MinGW)
```bash
./vcpkg/bootstrap-vcpkg.sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$PWD/cmake/mingw-toolchain.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
*(To create a release package locally, run `cd build && cpack`)*
</details>

---

## Usage

Run the tool while **the game** is actively running. 

If you downloaded the `.zip` release, simply execute the launch script from the root folder:
- **Windows:** `.\launch.bat` *(Run as Administrator)*
- **Linux:** `./launch_linux.sh` *(Requires `protontricks` to inject into the Steam Proton container)*
- **macOS:** `./launch_macos.sh` *(Requires Wine)*

### If you built from Source
The launch scripts are located inside the `scripts/` directory. Run them from the project root:
- **Windows:** `.\scripts\launch.bat` *(Run as Administrator)*
- **Linux:** `./scripts/launch_linux.sh` *(Requires `protontricks` to inject into the Steam Proton container)*
- **macOS:** `./scripts/launch_macos.sh` *(Requires Wine)*

---

## Support & Contributing

Suggestions and contributions are always welcome! If you encounter any issues, bugs, or have any idea for a new feature, please use the **Issues** tab below. If you enjoy using this tool and want to support its development, consider buying me a coffee via [PayPal](https://www.paypal.com/paypalme/MuhamadSyakir)

- **[Report a Bug](../../issues/new?template=bug-report.yml)**
- **[Request a Feature](../../issues/new?template=feature-request.yml)**
- **[Ask a Question](../../discussions)**

---

## License
This project is licensed under the [MIT License](license).