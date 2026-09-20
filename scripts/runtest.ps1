<#
.SYNOPSIS
    Build, stage, run and debug redalert.exe against the owner's OWN legally-owned Red Alert data.

.DESCRIPTION
    Wraps the workflow documented in Agent/knowledge_base/debugging_and_runtime_testing.md.
    Windows PowerShell 5.1 compatible (no && / ternary). ASCII only on purpose.

      build   configure (if needed) and build the 'redalert' target
      setup   create the test directory and hard-link the game data into it (no 450MB copy)
      deploy  copy redalert.exe/.pdb from the build tree into the test directory
      run     deploy, launch, and report whether it stayed alive or crashed within -TimeoutSec
      debug   deploy, launch under cdb, run to the first stop (crash) and print the call stack

    The game data is NEVER copied into the repo and NEVER downloaded - point -SteamDataDir at
    the classic CD1 folder of your own Steam "CnC Remastered Collection" install.

.EXAMPLE
    .\scripts\runtest.ps1 build
    .\scripts\runtest.ps1 setup
    .\scripts\runtest.ps1 run
    .\scripts\runtest.ps1 debug
    .\scripts\runtest.ps1 debug -CdbScript C:\path\to\my.cdb     # custom cdb command file
#>
param(
    [Parameter(Position = 0)]
    [ValidateSet('build', 'setup', 'deploy', 'run', 'debug')]
    [string]$Action = 'run',

    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug',   # Release has NO pdb in this project's CMake config - use Debug to debug

    [string]$SteamDataDir = 'C:\Program Files (x86)\Steam\steamapps\common\CnCRemastered\Data\CNCDATA\RED_ALERT\CD1',

    [string]$TestDir = '',       # default: <repo>\build\runtest  (build/ is git-ignored)

    [int]$TimeoutSec = 20,

    [string]$CdbScript = '',     # optional cdb command file; default = run to first stop, print stack

    [switch]$KeepRunning         # 'run' only: do not kill the game if it is still alive at the timeout
)

$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ($TestDir -eq '') { $TestDir = Join-Path $Repo 'build\runtest' }
$BuildDir = Join-Path $Repo 'build\Main'
$ExeSrc   = Join-Path $BuildDir ("CODE\{0}\redalert.exe" -f $Config)
$PdbSrc   = Join-Path $BuildDir ("CODE\{0}\redalert.pdb" -f $Config)
$Cdb      = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe'

# The 7 files the game needs from the classic CD1 folder (verified against a working setup).
$GameFiles = @('MAIN.MIX', 'REDALERT.MIX', 'EXPAND.MIX', 'EXPAND2.MIX', 'HIRES1.MIX', 'LORES1.MIX', 'REDALERT.INI')

function Do-Build {
    if (-not (Test-Path (Join-Path $BuildDir 'CMakeCache.txt'))) {
        Write-Host 'Configuring (cmake --preset Main) ...'
        Push-Location $Repo
        try { & cmake --preset Main; if ($LASTEXITCODE -ne 0) { throw 'cmake configure failed (is externals/vcpkg initialised? git submodule update --init)' } }
        finally { Pop-Location }
    }
    Write-Host ("Building redalert ({0}) ..." -f $Config)
    & cmake --build $BuildDir --target redalert --config $Config -- -m
    if ($LASTEXITCODE -ne 0) { throw 'build failed' }
}

function Do-Setup {
    if (-not (Test-Path $SteamDataDir)) {
        throw ("Game data folder not found: {0}`nPass -SteamDataDir pointing at YOUR OWN Red Alert CD1 data. This script never downloads anything." -f $SteamDataDir)
    }
    New-Item -ItemType Directory -Force -Path $TestDir | Out-Null
    foreach ($f in $GameFiles) {
        $src = Join-Path $SteamDataDir $f
        $dst = Join-Path $TestDir $f
        if (-not (Test-Path $src)) { throw ("Missing in game data folder: {0}" -f $src) }
        if ($f -like '*.INI') {
            # The game REWRITES its INI at startup. A hard link would silently modify the file inside
            # the owner's Steam install (this really happened once) - so the INI is always a private copy.
            if (Test-Path $dst) {
                if ((Get-Item $dst).LinkType -eq 'HardLink') { Remove-Item $dst -Force } else { continue }
            }
            Copy-Item $src $dst
            continue
        }
        if (Test-Path $dst) { continue }
        try {
            New-Item -ItemType HardLink -Path $dst -Target $src | Out-Null   # same NTFS volume required
        } catch {
            Write-Warning ("Hard link failed for {0} (different volume?) - copying instead." -f $f)
            Copy-Item $src $dst
        }
    }
    Write-Host ("Test directory ready: {0}" -f $TestDir)
}

function Do-Deploy {
    if (-not (Test-Path $ExeSrc)) { throw ("Not built: {0}  (run: runtest.ps1 build)" -f $ExeSrc) }
    if (-not (Test-Path (Join-Path $TestDir 'MAIN.MIX'))) { Do-Setup }
    Copy-Item $ExeSrc (Join-Path $TestDir 'redalert.exe') -Force
    if (Test-Path $PdbSrc) { Copy-Item $PdbSrc (Join-Path $TestDir 'redalert.pdb') -Force }
}

function Do-Run {
    Do-Deploy
    Get-Process redalert -ErrorAction SilentlyContinue | Stop-Process -Force
    $p = Start-Process -FilePath (Join-Path $TestDir 'redalert.exe') -WorkingDirectory $TestDir -PassThru
    $exited = $p.WaitForExit($TimeoutSec * 1000)
    if ($exited) {
        $code = $p.ExitCode
        Write-Host ("EXITED within {0}s, exit code 0x{1:X8}" -f $TimeoutSec, ($code -band 0xFFFFFFFF))
        if (($code -band 0xFFFFFFFF) -eq 0xC0000005) { Write-Host 'Access violation - run: runtest.ps1 debug   (prints the symbolized stack)' }
        exit 1
    }
    Write-Host ("STILL ALIVE after {0}s (pid {1})." -f $TimeoutSec, $p.Id)
    if (-not $KeepRunning) { $p.Kill(); Write-Host 'Killed (use -KeepRunning to leave it up).' }
}

function Do-Debug {
    if (-not (Test-Path $Cdb)) { throw "cdb.exe not found - install the 'Debugging Tools for Windows' part of the Windows SDK." }
    if ($Config -ne 'Debug') { Write-Warning 'Release builds have no pdb here; stacks will not be symbolized. Use -Config Debug.' }
    Do-Deploy
    if ($CdbScript -eq '') {
        $CdbScript = Join-Path $TestDir 'default.cdb'
        # No -g on the cdb command line: the script must run at the initial breakpoint so that
        # breakpoints/watchpoints can be armed BEFORE the game runs. 'g' then runs to the first stop.
        Set-Content -Path $CdbScript -Encoding ASCII -Value @('.lines', 'g', '.echo ==STOPPED', '.exr -1', 'k 25', 'q')
    }
    $log = Join-Path $TestDir 'cdb.log'
    $args = @('-G', '-o', '-y', (Join-Path $BuildDir ("CODE\{0}" -f $Config)), '-cf', $CdbScript, (Join-Path $TestDir 'redalert.exe'))
    $p = Start-Process -FilePath $Cdb -ArgumentList $args -WorkingDirectory $TestDir -RedirectStandardOutput $log -NoNewWindow -PassThru
    if (-not $p.WaitForExit($TimeoutSec * 1000)) {
        Write-Host ("Debuggee still running after {0}s (no crash) - killing cdb + game." -f $TimeoutSec)
        $p.Kill()
        Get-Process redalert -ErrorAction SilentlyContinue | Stop-Process -Force
    }
    Write-Host ("Full log: {0}" -f $log)
    $lines = Get-Content $log | Where-Object { $_ -notmatch '^ModLoad:|NatVis' }
    $i = [array]::IndexOf($lines, ($lines | Where-Object { $_ -match '==STOPPED' } | Select-Object -First 1))
    if ($i -ge 0) { $lines[$i..($lines.Count - 1)] } else { $lines | Select-Object -Last 40 }
}

switch ($Action) {
    'build'  { Do-Build }
    'setup'  { Do-Setup }
    'deploy' { Do-Deploy }
    'run'    { Do-Run }
    'debug'  { Do-Debug }
}
