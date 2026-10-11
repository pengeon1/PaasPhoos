Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "       PaasPhoos One-Command Build Script (PowerShell)" -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] CMake not found in PATH." -ForegroundColor Red
    exit 1
}

Write-Host "[*] Configuring CMake project (Release)..." -ForegroundColor Yellow
cmake -B build -S .
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "[*] Building PaasPhoos (Release)..." -ForegroundColor Yellow
cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Green
Write-Host "[SUCCESS] PaasPhoos build completed successfully!" -ForegroundColor Green
Write-Host "Look for PaasPhoos.exe in build\PaasPhoos_artefacts\Release or build\Release" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
