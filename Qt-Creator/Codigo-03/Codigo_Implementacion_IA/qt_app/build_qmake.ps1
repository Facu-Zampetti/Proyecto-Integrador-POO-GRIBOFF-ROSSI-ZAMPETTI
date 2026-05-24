param(
    [ValidateSet("release", "debug")]
    [string]$Configuration = "release"
)

$ErrorActionPreference = "Stop"

$qtRoot = "C:\Qt\6.11.0\mingw_64"
$toolsRoot = "C:\Qt\Tools\mingw1310_64\bin"

$qmakeExe = Join-Path $qtRoot "bin\qmake.exe"
$deployExe = Join-Path $qtRoot "bin\windeployqt.exe"
$makeExe = Join-Path $toolsRoot "mingw32-make.exe"

foreach ($tool in @($qmakeExe, $deployExe, $makeExe)) {
    if (-not (Test-Path $tool)) {
        throw "No se encontro la herramienta requerida: $tool"
    }
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build-qmake"
$proFile = Join-Path $scriptDir "CarlensQtRunner.pro"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

$env:Path = "$toolsRoot;$($qtRoot)\bin;" + $env:Path

Push-Location $buildDir
try {
    & $qmakeExe $proFile "CONFIG+=$Configuration"
    & $makeExe

    $binDir = Join-Path $buildDir "bin"
    $exePath = Join-Path $binDir "CarlensQtRunner.exe"
    if (-not (Test-Path $exePath)) {
        throw "No se genero el ejecutable esperado: $exePath"
    }

    & $deployExe --no-translations --compiler-runtime $exePath
    Write-Host "Build y deploy completados: $exePath"
}
finally {
    Pop-Location
}