# Solaris History - ejecutar
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $root "build\SolarisHistory.exe"
if (-not (Test-Path $exe)) {
    Write-Host "No existe el ejecutable. Ejecuta primero .\build.ps1" -ForegroundColor Yellow
    exit 1
}
Start-Process -FilePath $exe -WorkingDirectory $root
