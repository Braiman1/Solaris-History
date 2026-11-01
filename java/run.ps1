# Solaris History - ejecuta las herramientas de contenido Java
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here
$jar = Join-Path $here "SolarisTools.jar"

$java = $null
if ($env:JAVA_HOME) {
    $cand = Join-Path $env:JAVA_HOME "bin\java.exe"
    if (Test-Path $cand) { $java = $cand }
}
if (-not $java) {
    $bundled = Join-Path $root ".tools\jdk21\jdk-21.0.12.1+1\bin\java.exe"
    if (Test-Path $bundled) { $java = $bundled }
}
if (-not $java) {
    $cmd = Get-Command java -ErrorAction SilentlyContinue
    if ($cmd) { $java = $cmd.Source }
}
if (-not $java) {
    Write-Host "ERROR: no se encontro Java 21." -ForegroundColor Red
    exit 1
}
if (-not (Test-Path $jar)) {
    Write-Host "No existe SolarisTools.jar. Ejecuta java\build.cmd primero." -ForegroundColor Yellow
    exit 1
}

Push-Location $root
try {
    if (-not $Arguments -or $Arguments.Count -eq 0) {
        & $java -jar $jar stats
    } else {
        & $java -jar $jar @Arguments
    }
} finally {
    Pop-Location
}
