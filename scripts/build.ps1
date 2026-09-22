<#
.SYNOPSIS
  Configure + build (+ optionally test) the project with MSVC via Ninja.

  Why this script exists: the Ninja generator (unlike the Visual Studio
  project generator) does not auto-detect the MSVC environment. Running
  `cmake` directly in a plain PowerShell window fails to find cl.exe/link.exe
  unless you've sourced vcvars64.bat first. This script finds the installed
  Visual Studio via vswhere, sources vcvars64.bat in a cmd.exe subshell, and
  runs the cmake/ctest commands inside that environment.

  If you use VS Code with the CMake Tools extension instead, it detects the
  MSVC kit and handles this automatically - you don't need this script there.

.EXAMPLE
  ./scripts/build.ps1 -Preset debug -Test
  ./scripts/build.ps1 -Preset release
#>
param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",
    [switch]$Clean,
    [switch]$Test
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found - is Visual Studio Build Tools installed?"
}
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) {
    throw "No Visual Studio install with the C++ workload was found."
}
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"

if ($Clean) {
    Remove-Item -Recurse -Force "$repoRoot\build\$Preset" -ErrorAction SilentlyContinue
}

$vswhereDir = Split-Path -Parent $vswhere
$buildCmd = "cmake --preset $Preset && cmake --build --preset $Preset"
cmd /c "set `"PATH=%PATH%;$vswhereDir`" && `"$vcvars`" >nul && cd /d `"$repoRoot`" && $buildCmd"
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)" }

if ($Test) {
    cmd /c "set `"PATH=%PATH%;$vswhereDir`" && `"$vcvars`" >nul && cd /d `"$repoRoot`" && ctest --preset $Preset"
    if ($LASTEXITCODE -ne 0) { throw "Tests failed (exit $LASTEXITCODE)" }
}
