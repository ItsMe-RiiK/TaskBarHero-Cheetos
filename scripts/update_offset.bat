@echo off
setlocal

:: Navigate to the project root directory
cd /d "%~dp0.."

set "DUMP_FILE=resources\dump\dump.cs"
set "HEADER_FILE=src\core\Il2CppOffsets.h"
set "UPDATER_SCRIPT=src\tools\UpdateOffsets.py"

echo ===========================================
echo   TaskBarHero - Offset Auto-Updater
echo ===========================================

if not exist "%DUMP_FILE%" (
    echo [!] Error: Dump file not found at '%DUMP_FILE%'
    echo [!] Please run Il2CppDumper first and place 'dump.cs' in the resources\dump\ folder!
    exit /b 1
)

if not exist "%UPDATER_SCRIPT%" (
    echo [!] Error: Updater script not found at '%UPDATER_SCRIPT%'
    exit /b 1
)

echo [*] Found dump.cs! Extracting offsets...
python "%UPDATER_SCRIPT%" "%DUMP_FILE%" "%HEADER_FILE%"

echo.
echo [*] Done! Don't forget to rebuild the project.