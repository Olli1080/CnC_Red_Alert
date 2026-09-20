---
name: debugging-and-runtime-testing
description: How to build redalert.exe, stage the owner's game data, run it, and debug crashes with cdb - including the cdb recipes and gotchas learned the hard way
metadata:
  type: knowledge_base
---

# Building, running and debugging `redalert.exe`

Everything here is wrapped by [`scripts/runtest.ps1`](../../scripts/runtest.ps1) (Windows PowerShell 5.1). Read the script - it is short - and use it rather than retyping commands.

## Prerequisites (this machine has them; a fresh one needs them)
- Visual Studio 2026 (generator string `Visual Studio 18 2026`, MSVC 19.51, x64 only), CMake, and the vcpkg submodule: `git submodule update --init` (lives at `externals/vcpkg`; already bootstrapped on the dev machine).
- Windows SDK "Debugging Tools for Windows": `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`.
- **The owner's own copy of Red Alert.** The classic game data ships inside Steam's *Command & Conquer Remastered Collection*: `C:\Program Files (x86)\Steam\steamapps\common\CnCRemastered\Data\CNCDATA\RED_ALERT\CD1\`. Needed files: `MAIN.MIX REDALERT.MIX EXPAND.MIX EXPAND2.MIX HIRES1.MIX LORES1.MIX REDALERT.INI`. Never copy game data into the repo, never commit it, never download it (see [../agent_policies/licensing.md](../agent_policies/licensing.md)).

## Quick start
```powershell
.\scripts\runtest.ps1 build      # configure (first time) + build the redalert target, Debug
.\scripts\runtest.ps1 setup      # build\runtest\ with hard-linked MIX files + a private REDALERT.INI copy
.\scripts\runtest.ps1 run        # launch; prints "EXITED ... 0xC0000005" (crash) or "STILL ALIVE"
.\scripts\runtest.ps1 debug      # same, under cdb: prints the exception + symbolized stack
```
Use **`-Config Debug`** (the default): the Release configuration produces no PDB in this project's CMake setup, so stacks aren't symbolized. Debug also links the debug CRT (`VCRUNTIME140D.dll`/`ucrtbased.dll`) - fine on a machine with Visual Studio.

`build/` is git-ignored, so `build\runtest\` is safe scratch space. Manual build equivalent: `cmake --build build/Main --target win32lib redalert_code redalert --config Debug -- -m` (a full rebuild after touching a widely-included header takes several minutes; MSBuild output is German on this machine but error lines still contain `error C####`).

### Two things that already bit us
1. **The game rewrites `REDALERT.INI` at startup.** The MIX files are only read, so they are hard-linked (no 450 MB copy), but the INI **must be a private copy** - hard-linking it once modified the file inside the owner's Steam install (restored by hand afterwards). `runtest.ps1 setup` already does this correctly.
2. **The game window is a full-screen, always-on-top popup** (`WS_POPUP|WS_EX_TOPMOST`, `CODE/WINSTUB.CPP`). If a run hangs it can cover the screen: `taskkill /F /IM redalert.exe`. It also flashes black for a moment before crashing - that is the D3D11 swapchain window appearing, not a bug in itself.

## cdb recipes
Run cdb as `cdb -G -o -y <build\Main\CODE\Debug> -cf script.cdb redalert.exe` (this is what `runtest.ps1 debug` does; `-CdbScript` lets you pass your own file). Notes that cost hours:

- **Do NOT pass `-g`** if the script must set breakpoints first: `-g` skips the initial breakpoint, so the command file only starts executing at the *first exception* (after the crash). Without `-g` it runs at process start; then `g` runs to the first stop.
- Symbols: `-y <dir containing redalert.pdb>`. The exe's embedded PDB path is the original build path, so pointing `-y` at `build\Main\CODE\Debug` always works even for a copied exe. Stale PDB = "symbols could not be loaded"; always deploy exe+pdb together.
- The stack top frame is often inside the CRT (`memmove`/`memset`) - the interesting frame is the *first* `redalert!` frame below it, with `@ file.cpp line` shown thanks to `.lines`.
- Handy one-liners at a stop: `.exr -1` (exception record; `Parameter[1]` is the faulting address), `.frame N; dv /t /v` (locals), `dt redalert!ClassName` (layout/offsets), `?? this`, `?? *this`, `u @rip-0x24 L14` (disassembly around the fault), `r`.
- **Hardware write-watchpoint = fastest way to find a memory corruptor** (this found the `#pragma pack` bug, see [pragma_pack_leak_odr.md](pragma_pack_leak_odr.md)):
  ```
  bp redalert!Init_Keys ; g                    * get to a known function first
  bp redalert!INIClass::INIClass ; g           * break at the object's constructor (rcx = this)
  r $t0 = @rcx + 0x38                          * address of the member (offset from `dt`)
  ba w8 @$t0 ".echo ==WRITE; u @rip-0x18 L8; dq @$t0 L1; g"   * address MUST be 8-aligned for w8
  g
  ```
  The trap fires *after* the store retires, so `u @rip-0x18` shows the store instruction itself.
- **C++ template symbol names are awkward in cdb**: `x` prints `IndexClass<INIClass::INISection *>::Is_Archive_Same` (with a space before `*`) but `bp`/`uf` on that text often fail with "Couldn't resolve". Break on a *non-template* function that reaches the code (e.g. `redalert!INIClass::INIClass`), or use `x redalert!IndexClass<*>::*` to list addresses and `bp <address>` (addresses move with ASLR, so re-list each run).
- Attaching to an already-running instance: `-pv` = read-only/non-invasive (can inspect, cannot resume/set breakpoints); plain `-p` = invasive (killing cdb then kills the game; use `qd` to detach). Launching under cdb from the start (above) is almost always easier.
- If Visual Studio's JIT debugger pops up on a crash and grabs the process, kill `devenv`/`msvsmon` - it blocks `cdb`.

## Getting a crash location *without* a debugger
`Get-WinEvent -FilterHashtable @{LogName='Application'; ProviderName='Application Error'} -MaxEvents 1` gives the faulting module + offset (`0xC0000005` = access violation). Useful to confirm a run crashed, useless for *where* - use `cdb`.

## Seeing what the game draws
- Screenshots taken from PowerShell (`CopyFromScreen`) frequently capture **the agent app's own window, not the game** - the game is a topmost popup that may already have exited. Do not conclude anything from such a screenshot unless the game process is verifiably alive at that moment (`Get-Process redalert`). **The user watching the screen is ground truth** ("it flashed black for a split second" was the correct observation).
- Until the boot sequence reaches the main menu there is nothing to see; use the `cdb` stack as the progress metric.

## What "progress" looks like (boot stages, in order)
`WinMain` -> DirectDraw/D3D11 init (`Set_Video_Mode`, `InitDDraw` path in `STARTUP.CPP`) -> `Main_Game` -> `Init_Game` -> `Init_Keys` (INI/RSA key) -> `Bootstrap` (MIX files, fonts, palette) -> `Init_One_Time_Systems` ... -> menus. See [../HANDOFF.md](../HANDOFF.md) for exactly which stage currently fails.
