# Solaris History - compilacion con CMake + MinGW
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root ".tools\build-windows"

Write-Host "== Solaris History: compilando ==" -ForegroundColor Yellow

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: CMake no esta en el PATH." -ForegroundColor Red
    exit 1
}

$generator = "MinGW Makefiles"
if (-not (Get-Command mingw32-make -ErrorAction SilentlyContinue) -and -not (Get-Command make -ErrorAction SilentlyContinue)) {
    if (Get-Command ninja -ErrorAction SilentlyContinue) { $generator = "Ninja" }
    else {
        Write-Host "ERROR: se necesita mingw32-make, make o ninja." -ForegroundColor Red
        exit 1
    }
}

cmake -S (Join-Path $root "engine") -B $build -G $generator -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Join-Path $build "SolarisHistory.exe"
if (Test-Path $exe) {
    Write-Host "== Listo: $exe ==" -ForegroundColor Green
} else {
    Write-Host "ERROR: no se genero SolarisHistory.exe" -ForegroundColor Red
    exit 1
}
