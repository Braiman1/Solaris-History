# Solaris History - compila las herramientas de contenido Java
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here
$out = Join-Path $here "out"
$jar = Join-Path $here "SolarisTools.jar"

function Find-Java {
    if ($env:JAVA_HOME) {
        $cand = Join-Path $env:JAVA_HOME "bin\java.exe"
        if (Test-Path $cand) { return $cand }
    }
    $bundled = Join-Path $root ".tools\jdk21\jdk-21.0.12.1+1\bin\java.exe"
    if (Test-Path $bundled) { return $bundled }
    $cmd = Get-Command java -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$java = Find-Java
if (-not $java) {
    Write-Host "ERROR: no se encontro Java 21. Ejecuta tools\\fetch_jdk.ps1 o instala un JDK." -ForegroundColor Red
    exit 1
}
$javac = Join-Path (Split-Path -Parent $java) "javac.exe"
if (-not (Test-Path $javac)) {
    Write-Host "ERROR: no se encontro javac junto a $java" -ForegroundColor Red
    exit 1
}

Write-Host "== Solaris History: compilando herramientas Java ==" -ForegroundColor Yellow
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Path $out | Out-Null

$sources = Get-ChildItem -Path (Join-Path $here "src") -Recurse -Filter *.java | ForEach-Object { $_.FullName }
& $javac -encoding UTF-8 -d $out @sources
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& (Join-Path (Split-Path -Parent $java) "jar.exe") --create --file $jar --main-class solaris.tools.Main -C $out .
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== Listo: $jar ==" -ForegroundColor Green
Write-Host "Prueba: java -jar `"$jar`" validate"
