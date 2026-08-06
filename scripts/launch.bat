@echo off
setlocal
set "EXE_NAME=TBH-Cheetos.exe"
set "EXE_PATH="

:: Check current directory (Release mode / Zip package)
if exist "%~dp0%EXE_NAME%" (
    set "EXE_PATH=%~dp0%EXE_NAME%"
) else if exist "%~dp0build\%EXE_NAME%" (
    :: Check inside build directory (if script run from source root)
    set "EXE_PATH=%~dp0build\%EXE_NAME%"
) else if exist "%~dp0..\build\%EXE_NAME%" (
    :: Check from scripts dir looking into build
    set "EXE_PATH=%~dp0..\build\%EXE_NAME%"
) else (
    echo Error: %EXE_NAME% not found! Please build the project or place this script next to the executable.
    pause
    exit /b 1
)

echo Launching %EXE_PATH%...
"%EXE_PATH%"
pause
