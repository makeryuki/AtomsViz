param(
    [string]$Configuration = 'Debug',
    [string]$Platform = 'x64',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$solution = Join-Path $repoRoot 'Builds/VisualStudio2022/AtmosViz.sln'

if (-not (Test-Path $solution)) {
    throw "Solution file not found: $solution"
}

if (-not $SkipBuild) {
    $msbuildCandidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe"
    )

    $msbuild = $msbuildCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $msbuild) {
        throw 'MSBuild (amd64) executable not found. Update build_vst3.ps1 with the correct path.'
    }

    & $msbuild $solution /t:Build /p:Configuration=$Configuration /p:Platform=$Platform /m
}

$buildRoot = Join-Path $repoRoot "Builds/VisualStudio2022/$Platform/$Configuration/VST3"
$dllPath   = Join-Path $buildRoot 'AtmosViz.dll'
if (-not (Test-Path $dllPath)) {
    throw "AtmosViz.dll not found at $dllPath"
}

$bundleRoot = Join-Path $buildRoot 'AtmosViz.vst3'
$binaryDir  = Join-Path $bundleRoot 'Contents/x86_64-win'
$resourceDir = Join-Path $bundleRoot 'Contents/Resources'

New-Item -ItemType Directory -Force -Path $binaryDir | Out-Null
New-Item -ItemType Directory -Force -Path $resourceDir | Out-Null

Remove-Item (Join-Path $binaryDir 'AtmosViz.vst3') -ErrorAction SilentlyContinue
Copy-Item -Path $dllPath -Destination (Join-Path $binaryDir 'AtmosViz.vst3') -Force

$moduleInfoSource = Get-ChildItem -Path $buildRoot -Filter 'moduleinfo.json' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if ($moduleInfoSource) {
    $targetModulePath = Join-Path $resourceDir 'moduleinfo.json'
    if ($moduleInfoSource.FullName -ne $targetModulePath) {
        Copy-Item -Path $moduleInfoSource.FullName -Destination $targetModulePath -Force
    }
}

Write-Host "Created bundle: $bundleRoot"

