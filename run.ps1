# Solaris History - compilar los cambios y ejecutar la version actual
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $root "build.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$build = Join-Path $root ".tools\build-windows"
$exe = Join-Path $build "SolarisHistory.exe"
if (-not (Test-Path $exe)) {
    Write-Host "No se encontro el ejecutable compilado." -ForegroundColor Red
    exit 1
}
Start-Process -FilePath $exe -WorkingDirectory $build
