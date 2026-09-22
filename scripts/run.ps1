<#
.SYNOPSIS
  Run a built executable with the MSVC environment on PATH.

  Necessary for Debug/ASan builds: AddressSanitizer's runtime DLL
  (clang_rt.asan_dynamic-x86_64.dll) lives inside the MSVC toolset folder,
  not next to the exe or anywhere on the normal PATH. Launching an
  ASan-instrumented exe from a plain terminal fails immediately with
  STATUS_DLL_NOT_FOUND (exit code -1073741515 / 0xC0000135) and no other
  output - confirmed on this machine. Release builds don't hit this, but
  it's harmless to always run through this script.

.EXAMPLE
  ./scripts/run.ps1 build/debug/src/hft_server.exe
  ./scripts/run.ps1 build/debug/bench/hft_bench.exe --benchmark_min_time=0.1s
#>
param(
    [Parameter(Mandatory, Position = 0)]
    [string]$ExePath,
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$ExeArgs
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"

$vswhereDir = Split-Path -Parent $vswhere
$fullExePath = Join-Path $repoRoot $ExePath
$argString = ($ExeArgs -join " ")
cmd /c "set `"PATH=%PATH%;$vswhereDir`" && `"$vcvars`" >nul && `"$fullExePath`" $argString"
exit $LASTEXITCODE
