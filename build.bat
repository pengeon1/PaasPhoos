@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          PaasPhoos One-Command Build Script
echo =======================================================
echo.

where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake is not found in PATH!
    echo Please install CMake or run from a developer command prompt.
    exit /b 1
)

echo [*] Configuring CMake project (Release)...
cmake -B build -S .
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b %ERRORLEVEL%
)

echo.
echo [*] Building PaasPhoos (Release)...
cmake --build build --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo =======================================================
echo [SUCCESS] PaasPhoos build completed!
echo Executable is located in:
echo   build\PaasPhoos_artefacts\Release\PaasPhoos.exe (or build\Release\PaasPhoos.exe)
echo =======================================================
