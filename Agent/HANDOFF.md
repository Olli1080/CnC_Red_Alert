# HANDOFF - read this first

*Last updated 2026-11-07. If anything here disagrees with the code, trust the code and fix this file.*

## 1. Where the project stands (one paragraph)
This repo is EA's 2025 GPL release of Red Alert (1996, Watcom/TASM, 32-bit). It is being **modernized to build with CMake + vcpkg on MSVC 2026, x64 only** (see [plans_wip/modernization_cmake_vcpkg.md](plans_wip/modernization_cmake_vcpkg.md)). `win32lib.lib`, `redalert_code.lib` and **`redalert.exe` all build with 0 errors and 0 unresolved externals** (all ~66 remaining `.ASM` files were rewritten in C++; there is no assembler dependency). Run against the owner's own game data with `scripts/runtest.ps1`, the exe now **boots all the way to the main menu**: `Init_Game` completes, the title picture and the menu (buttons, text) are drawn through the D3D11 presenter with the correct palette, and the game idles in `Main_Menu`'s input loop. Milestones M1 and M2 of [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md) are reached; the work now is M3 (mouse cursor, button input), M4 (load a scenario, draw the map - exercises the unproven `Buffer_Frame_To_Page`/stamp code) and M5 (audio).

## 2. Status at a glance
| Stage | State |
|---|---|
| `win32lib` (1a), `redalert_code` (1b), `redalert.exe` links (1c) | done, committed |
| DirectDraw 8-bit wall | **worked around** by a Direct3D 11 presenter (`WIN32LIB/MISC/PRESENT_D3D.CPP`), committed `86f6b00`. Modern DirectDraw refuses *any* 8-bit surface (`DDERR_INVALIDPIXELFORMAT`), so `VisiblePage` falls back to plain system memory and D3D11 uploads it as an `R8_UINT` texture + a 256x1 palette texture; a 2-line pixel shader expands it |
| `Init_Keys` (INI + RSA key) | **fixed, committed `e75d5bf`**: leaked `#pragma pack` ([knowledge_base/pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md)) |
| `Bootstrap` (MIX parsing, `TEMPERAT.PAL`) | **fixed, committed `d1db96f`**: on-disk `MixFileClass::FileHeader` was 8 bytes instead of 6 (same document) |
| `Init_Mouse`, `Init_CDROM_Access`, `Init_Color_Remaps`, title picture, palette | **fixed** (see `git log`): `.SHP` and `CompHeaderType` packing, 64-bit pointer math in `2KEYFRAM.CPP`, uninitialized `display` in `MSGBOX.CPP`, 6-bit vs 8-bit palette scale in `PaletteClass::Set`. Details: [knowledge_base/x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md) #12-#15, [pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md) instances 3-4 |
| **Main menu** | **reached, renders, mouse works**: cursor is visible and follows the pointer over the whole window, click coordinates are correct (`MouseQX/QY` = 320,294 on "Exit Game"), and **"Exit Game" exits the process cleanly** (verified by a scripted click). Pitfalls #16 (mouse scale / `ClipCursor` / present) and #17 (`bool ReadyToQuit`). No hover highlight is by design. NOT yet verified: `Start New Game` and anything after it |
| Everything after the menu (scenario load, map, units, audio) | never executed |
| Presenter draw path (`Present_VisiblePage`, `Update_Palette_Texture`) | **runs and is correct** (displayed indices matched an independent decode exactly) |

**The one insight that will save you the most time:** Westwood compiled `CODE/` with `/zp1` (byte-packed structs, `CODE/MAKEFILE:113`); MSVC defaults to 8-byte alignment. Every on-disk/network/shared struct is potentially the wrong size. Two of the last three crashes were this. Read [knowledge_base/pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md) and run its audit *proactively*.

## 3. First 30 minutes
```powershell
git status                                   # expect a clean tree (everything below is committed)
.\scripts\runtest.ps1 build                  # Debug build of the redalert target (first time also runs cmake --preset Main)
.\scripts\runtest.ps1 setup                  # build\runtest\ : hard-linked MIX files + PRIVATE REDALERT.INI copy
.\scripts\runtest.ps1 debug                  # runs under cdb; with no crash it reports 'still running' after the timeout (expected now: the game sits in the main menu)
.\scripts\runtest.ps1 run -KeepRunning -TimeoutSec 20   # leave the window open for the owner to look at
```
`runtest.ps1` passes `-CD.` to the game by default (`-GameArgs`), which makes it look for data in the working directory instead of a physical CD drive (without it `Force_CD_Available` pops the 'insert CD' dialog). Python on this machine: `python` is 2.7, use `py -3`. Prereqs and cdb recipes (write-watchpoints, template-symbol quirks, the `-g` trap): [knowledge_base/debugging_and_runtime_testing.md](knowledge_base/debugging_and_runtime_testing.md). Then open [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md) and continue at 'Next concrete steps' at its end.

## 4. Fixes made (verify with `git log`). Newest session first
Session 2026-11-07: `SHAPE.H` x2 packed; `IFF.H` x2 `CompHeaderType` packed; `2KEYFRAM.CPP`/`FUNCTION.H`/`CONQUER.CPP` 64-bit-safe shape cache (`Build_Frame` returns `uintptr_t`); `MSGBOX.CPP` `display` initialised; `PALETTE.CPP` raw 6-bit palette; `WINSTUB.CPP` unsigned palette shift; `scripts/runtest.ps1` `-GameArgs` (`-CD.`). Earlier session:
| Commit | What / why |
|---|---|
| `e75d5bf` | `#pragma pack` scoped with push/pop in `AUDIO.H`/`SOUNDINT.H` (both copies) - fixes the `IndexClass` crash in `Init_Keys` |
| `d1db96f` | `MixFileClass::FileHeader` packed to 6 bytes + `static_assert` - fixes `Bootstrap`'s NULL `Retrieve` |
| `eacb07d` | `Set_DD_Palette` fills `PaletteEntries[]` unconditionally (was gated on a NULL `PaletteSurface` => black screen forever); `Wait_Blit` NULL guard. **Builds; runtime path not yet reached** |
| `ef0fee9` | `scripts/runtest.ps1` + this documentation set |
The first two were verified by re-running (the crash moved). The third is reasoned and builds but its code path has not executed yet.

## 5. Commit history you'll want
`61244a5` sub-milestone 1c (links) - `86f6b00` D3D11 presenter + `Offset`->`INT_PTR` + 16-site pixel-pointer fix. Older: `82abd41` 1b, `356b3b5`/`3fde7e6` 1a, `9cc6035` Phase 0. Full narratives: [plans_done/](plans_done/summary.md) (`code_1c_port_log.md` is the richest; it now has the whole D3D11 story).

## 6. Where things are
| Need | Go to |
|---|---|
| What each module does | [project_state/](project_state/summary.md) |
| Build/run/debug how-to | [knowledge_base/debugging_and_runtime_testing.md](knowledge_base/debugging_and_runtime_testing.md), `scripts/runtest.ps1` |
| Bug classes found so far (triage checklist) | [knowledge_base/x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md) |
| The struct-packing saga | [knowledge_base/pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md) |
| Active work plan | [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md) |
| Big-picture roadmap (Phases 0-5; Phase 4 = SDL3 + Vulkan/D3D12 rewrite is the long-term goal) | [plans_wip/modernization_cmake_vcpkg.md](plans_wip/modernization_cmake_vcpkg.md) |
| Licensing / what you may do | [agent_policies/](agent_policies/summary.md) |
Key code: `CODE/STARTUP.CPP` (`WinMain`; *live* video init is inlined there under `#else` of `#ifdef MPEGMOVIE`), `CODE/INIT.CPP` (`Init_Game` and the whole boot sequence), `WIN32LIB/MISC/{DDRAW,PRESENT_D3D}.CPP` (video), `WIN32LIB/DRAWBUFF/` (blit primitives, all C++ rewrites of ASM), `CODE/CMakeLists.txt` (explicit source list transcribed from Westwood's own MAKEFILE - **not** a glob) vs `WIN32LIB/CMakeLists.txt` (globs per module dir).

## 7. Rules and boundaries (please keep)
- **Game data belongs to the owner.** Only ever use their own Steam copy of the CnC Remastered Collection (`...\CnCRemastered\Data\CNCDATA\RED_ALERT\CD1\`). Never commit it, redistribute it, or download replacements/assets. See [agent_policies/licensing.md](agent_policies/licensing.md).
- **Do not download or install third-party executables** (DirectDraw shims like dgVoodoo2/dxwrapper, cracks, "fixed" DLLs). A previous attempt was flagged by the owner's antivirus and abandoned; the D3D11 presenter exists precisely so none is needed. Don't suggest ways around the antivirus.
- **`REDALERT.INI` must be a private copy**, never a hard link: the game rewrites it on every start, and hard-linking once modified the file inside the owner's Steam install (it was restored by hand). `scripts/runtest.ps1 setup` does this correctly - use it.
- Commit messages follow the existing style (`git log`): a short imperative subject, then a body explaining *why*. Never force-push; upstream (`electronicarts/CnC_Red_Alert`) takes no PRs - `origin` is the owner's fork. Only commit/push when the owner asks (they usually do, at milestones).
- Duplicate headers: `WIN32LIB/INCLUDE/X.H` and `WIN32LIB/<MODULE>/X.H` (and a few in `CODE/`) exist; the include order decides which compiles. Edit both, then `diff`. `SRCDEBUG/`, `OLD*/`, `TEST/`, `*.BAK` are dead copies.

## 8. Working style that has paid off here (and what has not)
- **Verify by running.** "It links" was true for weeks while none of the rewritten blit code had ever executed. Use the cdb stack as the progress metric; report progress as "the crash moved from X to Y".
- **Root-cause, don't patch symptoms.** Nearly every crash so far was one of a handful of bug classes ([x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md)). A hardware write-watchpoint (`ba w8`) and reading the actual disassembly resolved in minutes what hours of reading source did not.
- **Trust the user's eyes over your screenshots.** PowerShell screenshots often capture the agent app instead of the (topmost, short-lived) game window. The owner reported "it went full-screen black for a split second" - that was the ground truth.
- Keep per-crash notes in the docs as you go; the owner explicitly wants findings recorded (their standing instruction: "document and commit up to here" at milestones).
- Environment quirks: German-locale Windows 11, Intel iGPU, VS 2026 (MSBuild messages are German; `error C####` tags are not). The machine once did a hardware "safety shutdown" during a test run (most likely thermal/power, unrelated to the code as far as anyone can tell) - the script kills the game after a timeout, keep runs short.

## 9. Known unknowns
- Everything in [x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md) #0's "never executed" column is unproven.
- `COORD.CPP` `calcx`/`calcy` and the `PREDATOR` effect's negative-offset table variant in `Buffer_Frame_To_Page` were flagged as not bit-exact-verified in 1c.
- `SOSCODEC.CPP` IMA-ADPCM decode is a silence stub by design; `SCOMP_WESTWOOD` audio is real. Movies (VQA/MPEG) and WOL/modem multiplayer are stubbed/excluded.
- ~40 C4311/C4312 pointer-truncation warnings remain (`grep` command in the pitfalls doc) - a crash forecast, not yet triaged.
- `IndexClass`-style template ODR bugs could exist for *other* packed/unpacked mismatches; the `static_assert(sizeof)` habit is the cheap defence.
