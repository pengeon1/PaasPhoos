# PaasPhoos Portable Distribution Packaging Script
$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
if (-not $projectRoot) { $projectRoot = Get-Location }

$buildArtefacts = Join-Path $projectRoot "build\PaasPhoos_artefacts\Release"
$buildBin = Join-Path $projectRoot "build\Release"
$distDir = Join-Path $projectRoot "dist"
$packageDir = Join-Path $distDir "PaasPhoos-v0.2.0-Windows-x64"

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Packaging PaasPhoos Portable Release Bundle" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

# 1. Clean and prepare dist directory
if (Test-Path $packageDir) {
    Remove-Item -Recurse -Force $packageDir
}
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

# 2. Copy Executables
$mainExe = Join-Path $buildArtefacts "PaasPhoos.exe"
if (-not (Test-Path $mainExe)) {
    $mainExe = Join-Path $buildBin "PaasPhoos.exe"
}

if (Test-Path $mainExe) {
    Copy-Item -Path $mainExe -Destination $packageDir -Force
    Write-Host "[OK] Copied PaasPhoos.exe" -ForegroundColor Green
} else {
    Write-Error "Could not find PaasPhoos.exe in build directories. Please build the project first."
}

$botExe = Join-Path $buildBin "PaasPhoosBot.exe"
if (Test-Path $botExe) {
    Copy-Item -Path $botExe -Destination $packageDir -Force
    Write-Host "[OK] Copied PaasPhoosBot.exe" -ForegroundColor Green
}

$testsExe = Join-Path $buildBin "PaasPhoosTests.exe"
if (Test-Path $testsExe) {
    Copy-Item -Path $testsExe -Destination $packageDir -Force
    Write-Host "[OK] Copied PaasPhoosTests.exe" -ForegroundColor Green
}

# 3. Copy Documentation & Resources
Copy-Item -Path (Join-Path $projectRoot "README.md") -Destination $packageDir -Force
Copy-Item -Path (Join-Path $projectRoot "ARCHITECTURE.md") -Destination $packageDir -Force
Copy-Item -Path (Join-Path $projectRoot "TESTING.md") -Destination $packageDir -Force

$resFolder = Join-Path $projectRoot "resources"
if (Test-Path $resFolder) {
    Copy-Item -Recurse -Path $resFolder -Destination $packageDir -Force
    Write-Host "[OK] Copied resources folder (logo, assets)" -ForegroundColor Green
}

# 4. Create Quick Start batch file
$quickStartContent = @"
@echo off
title PaasPhoos Portable Studio
start "" "%~dp0PaasPhoos.exe"
"@
Set-Content -Path (Join-Path $packageDir "Start-PaasPhoos.bat") -Value $quickStartContent

# 5. Compress to ZIP archive
$zipOutput = Join-Path $distDir "PaasPhoos-v0.2.0-Windows-x64.zip"
if (Test-Path $zipOutput) {
    Remove-Item -Force $zipOutput
}

Write-Host "Creating portable archive $zipOutput..." -ForegroundColor Yellow
Compress-Archive -Path "$packageDir\*" -DestinationPath $zipOutput -Force
Write-Host "[SUCCESS] Portable package created at: $zipOutput" -ForegroundColor Green
