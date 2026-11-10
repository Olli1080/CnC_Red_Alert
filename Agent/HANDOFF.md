# HANDOFF - read this first

*Last updated 2026-11-10. If anything here disagrees with the code, trust the code and fix this file.*

## 1. Where the project stands (one paragraph)
This repo is EA's 2025 GPL release of Red Alert (1996, Watcom/TASM, 32-bit). It is being **modernized to build with CMake + vcpkg on MSVC 2026, x64 only** (see [plans_wip/modernization_cmake_vcpkg.md](plans_wip/modernization_cmake_vcpkg.md)). `win32lib.lib`, `redalert_code.lib` and **`redalert.exe` build with 0 errors and 0 unresolved externals** (all ~66 remaining `.ASM` files were rewritten in C++; no assembler dependency). Run against the owner's own game data with `scripts/runtest.ps1`, the game is now **playable from the main menu through the first mission**: menu with working mouse -> Start New Game (Allies) -> mission 1 (map, units, sidebar, correct colours, music, scroll speed all confirmed by the owner) -> mission end -> score screen -> campaign map animation (confirmed 2026-11-07). **Open, in priority order:** (1) **almost all sound effects are missing** (owner report 2026-11-10; music and the sidebar click work) - this is the next task; (2) movies are skipped (`VQASTUB.CPP`) - options and the owner's preferred route in [plans_wip/movies.md](plans_wip/movies.md); (3) mission 2 and later, save/load, skirmish, speech are unverified.

## 2. Status at a glance
| Stage | State |
|---|---|
| `win32lib` (1a), `redalert_code` (1b), `redalert.exe` links (1c) | done, committed |
| DirectDraw 8-bit wall | **worked around** by a Direct3D 11 presenter (`WIN32LIB/MISC/PRESENT_D3D.CPP`). Modern DirectDraw refuses *any* 8-bit surface, so `VisiblePage` is plain system memory that D3D11 uploads as an `R8_UINT` texture + a 256x1 palette texture; a 2-line pixel shader expands it. Game 640x400 is stretched over the borderless popup `MainWindow` (e.g. 1920x1080) |
| Boot (`Init_Keys`, `Bootstrap`, `Init_Mouse`, palette, title, `Init_Game`) | done (pitfalls #3, #12-#15, [pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md)) |
| **Main menu** | works: cursor, click coordinates, "Exit Game" exits cleanly (#16, #17). The CD prompt is bypassed for `-CD<path>` runs (`Force_CD_Available`). No hover highlight by design |
| **Start New Game -> mission 1** | works, **verified by the owner**: scenario loads, logic runs, map/units/sidebar render with correct colours (#21), input works, scrolling speed OK after 60 Hz pacing (#24) |
| Mission end, score screen, **campaign map animation** | works, **verified by the owner** (#25: four WSA/LCW bugs) |
| Music | works (real SOS IMA-ADPCM decoder, #22; verified by ear) |
| **Sound effects** | **mostly missing** (owner 2026-11-10). Only the sidebar click was ever looked at (its crash fixed, #23). Path to inspect: `Play_Sample` -> `Sample_Copy`/`Decompress_Frame` (`SCOMP_WESTWOOD`, `AUDUNCMP.CPP`) -> `SOUNDINT.CPP` / `SOUNDIO.CPP` mixer. Start by logging which `Play_Sample` calls happen and what `Sample_Copy` returns |
| Movies (intro, briefings, cutscenes) | skipped: `CODE/VQASTUB.CPP` stubs `VQA_*`. The owner's data has only 2 classic VQAs; everything else is Remastered Bink 2 - see [plans_wip/movies.md](plans_wip/movies.md) |
| Mission 2+, save/load, skirmish/multiplayer, speech/EVA, alt-tab/device loss | unverified. WOL/modem multiplayer is excluded |
| Presenter draw path | runs and is correct (displayed indices matched an independent decode exactly) |

**The two insights that will save you the most time:**
1. Westwood compiled `CODE/` with `/zp1` (byte-packed structs, `CODE/MAKEFILE:113`) but `WIN32LIB` with Watcom's default `/zp2`; MSVC defaults to 8-byte alignment. Every on-disk/network/shared struct is potentially the wrong size ([pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md), pitfalls #3, #25).
2. Watcom differences beyond packing keep producing crashes that look unrelated: minimal-size enums in file formats (#20), writable string literals (#18), 32-bit `long`/pointers (#1, #12, #23), `bool` that held a state (#17), static-init order (#21), stubbed ASM replacements (#22), ASM->C++ argument-semantics mismatches (#25). Walk [knowledge_base/x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md) before theorising about the next crash.

## 3. First 30 minutes
```powershell
git status                                   # expect a clean tree (everything below is committed)
.\scripts\runtest.ps1 build                  # Debug build of the redalert target (first time also runs cmake --preset Main)
.\scripts\runtest.ps1 setup                  # build\runtest\ : hard-linked MIX files + PRIVATE REDALERT.INI copy
.\scripts\runtest.ps1 run -KeepRunning -TimeoutSec 20   # normal launch (no debugger), leave the window open for the owner
.\scripts\runtest.ps1 debug                  # under cdb; with -CdbScript <file> your own command file; the stack lands in build\runtest\cdb.log
```
`runtest.ps1` passes `-CD.` to the game by default (`-GameArgs`), so it looks for data in the working directory instead of a CD drive. Python on this machine: `python` is 2.7, use `py -3`. Prereqs and cdb recipes (write-watchpoints, template-symbol quirks, the `-g` trap, scripting gotchas): [knowledge_base/debugging_and_runtime_testing.md](knowledge_base/debugging_and_runtime_testing.md). Then continue with 'Next concrete steps' in [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md).

**Working protocol with the owner (they are the tester):** the owner plays; you start the game only when they say "start", and never start/kill/restart it while they are using it (they were disturbed by repeated launches). Launch plain (`run -KeepRunning`) for normal play or under cdb (`debug -CdbScript ... -TimeoutSec 1500`, in the background) when you want a crash stack; then read `build\runtest\cdb.log`. When they report something, ask *what happened* before changing code. **Do not make the owner replay a long path for a diagnostic: test your scripts first (on a throwaway cdb session) and, better, reproduce late-game code from an environment-gated startup self-test** (technique below) - three failed diagnostic runs in a row cost the owner's patience (2026-11-10).

## 4. What was fixed (verify with `git log`). Newest first
| Commit | What / why |
|---|---|
| `eae6e2f` | **Campaign-map animation crash**: `WSA_ORIGINAL_HEADER_SIZE` 44 -> 43 (in-place decode buffer was 1 byte short); `LCW_Uncompress_Bounded` honours `length` like `LCWUNCMP.ASM`; LCW long runs fill exactly `count` bytes (#25, 4th bug) |
| `d3c1c27` | `XORDELTA.CPP`: 4th parameter is the destination row stride, not "skip" (heap overflow, #25, 3rd bug) |
| `7c0f5ec` | WSA on-disk header packed to 22 bytes; header-size arithmetic for the 64-bit in-memory header (#25, 1st/2nd bug) |
| `9a8f785` | `Pace_To_Display()`: ~60 Hz pacing of the input/render loop (map scrolled absurdly fast, #24) |
| `eb0fdcd` | Pointer-truncation audit with `/w14311 /w14302 /w14312`; `RawFileClass::Read/Write` pointer advance, `VQAio` `uintptr_t`, `MapClass::Validate` heuristic (#23) |
| `5ddfcb3` | Grey screen = static-init order (`#pragma init_seg(lib)` in `FIXED.CPP`, #21); music silent = SOS ADPCM stub replaced by a real decoder (#22) |
| `b7ead81` | Writes into string literals fixed in the source (no writable `.rdata`, #18); stack-smashing palette index (#19); enum sizes (#20, `scripts/size_enums.py`) |
| `2d470af` | Start New Game loads the scenario and ticks the game logic |
| `897cae0` | Menu usable: mouse scale/`ClipCursor`/present (#16), `ReadyToQuit` bool -> int (#17), CD bypass |
| `52f2674`, `e0d0c10`, `e4a10fe` | Boot to the rendering main menu: `.SHP`/`CompHeaderType` packing, 64-bit shape cache, 6-bit palette scale (#12-#15) |
| `cc71819`, `ef0fee9`, `eacb07d`, `d1db96f`, `e75d5bf` | 2026-09-20: pragma-pack scoping in `AUDIO.H`/`SOUNDINT.H`, `MixFileClass::FileHeader` 6 bytes, palette reaching the presenter, `runtest.ps1` + this documentation set |
| `86f6b00`, `61244a5`, `82abd41`, `356b3b5`, `9cc6035` | D3D11 presenter; 1c/1b/1a; Phase 0 skeleton. Narratives: [plans_done/](plans_done/summary.md) (`code_1c_port_log.md` is the richest) |
Docs-only commits since: `11c9a6c`, `4a09888` (owner confirmations), `d1263b1`, `f6d1c40` (movies options), and the commit that carries this file.

## 5. Techniques that worked (details in the linked docs)
- **Hardware write breakpoint** finds a memory corruptor in one run (`ba w4/w8`, address must be aligned) - [debugging doc](knowledge_base/debugging_and_runtime_testing.md), pitfalls #25.
- **Environment-gated startup self-test**: for code that is only reachable after a long play-through, temporarily add `if (getenv("RA_X")) { call the entry points; log to a file; return; }` right after `Init_Game` in `Main_Game` (`CODE/CONQUER.CPP`), run it under cdb with the variable set, and compare outputs (e.g. decode twice: in place and from a separate copy). Remove it before committing (`grep -rn "TEMP-"`). This found the last WSA bug with zero owner replays.
- **Compiler as auditor**: `$env:CL='/w14311 /w14302 /w14312'` (pointer truncation) and `'/Zc:strictStrings'` (literal -> `char *`), then rebuild everything; remember `Remove-Item Env:CL`.
- **Compare against the original ASM next to the C++ rewrite** (`WIN32LIB/IFF/LCWUNCMP.ASM`, ...): check the *call sites'* argument semantics and any argument the C version calls a "dummy".
- Event log first, but check the faulting *module*: `Get-WinEvent` Application Error gives module + offset; resolve offsets with `cdb -cf resolve.cdb` only when the module is `redalert.exe` (pitfalls #18).

## 6. Where things are
| Need | Go to |
|---|---|
| What each module does | [project_state/](project_state/summary.md) |
| Build/run/debug how-to | [knowledge_base/debugging_and_runtime_testing.md](knowledge_base/debugging_and_runtime_testing.md), `scripts/runtest.ps1` |
| Bug classes found so far (triage checklist, #0-#25) | [knowledge_base/x64_porting_pitfalls.md](knowledge_base/x64_porting_pitfalls.md) |
| The struct-packing saga | [knowledge_base/pragma_pack_leak_odr.md](knowledge_base/pragma_pack_leak_odr.md) |
| Active work plan (next steps) | [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md) |
| Movies: options, owner's AV1 idea, data facts | [plans_wip/movies.md](plans_wip/movies.md) |
| Big-picture roadmap (Phases 0-5; Phase 4 = SDL3 + modern renderer is the long-term goal) | [plans_wip/modernization_cmake_vcpkg.md](plans_wip/modernization_cmake_vcpkg.md) |
| Investigations not started yet | [Analysis/summary.md](Analysis/summary.md) |
| Licensing / what you may do | [agent_policies/](agent_policies/summary.md) |
Key code: `CODE/STARTUP.CPP` (`WinMain`; the *live* video init is inlined there under `#else` of `#ifdef MPEGMOVIE`), `CODE/INIT.CPP` (`Init_Game`), `CODE/CONQUER.CPP` (`Main_Game`, `Sync_Delay`, `Play_Movie`), `WIN32LIB/MISC/{DDRAW,PRESENT_D3D}.CPP` (video, mouse mapping, pacing), `WIN32LIB/AUDIO/` (SOS codec, mixer), `WIN32LIB/WSA/` + `CODE/LCWUNCMP.CPP` (animations), `CODE/KEY.CPP` + `WIN32LIB/KEYBOARD/MOUSE.CPP` (the *active* input classes), `CODE/CMakeLists.txt` (explicit source list from Westwood's own MAKEFILE - **not** a glob) vs `WIN32LIB/CMakeLists.txt` (globs per module dir).

## 7. Rules and boundaries (please keep)
- **Game data belongs to the owner.** Only ever use their own Steam copy of the CnC Remastered Collection (`...\CnCRemastered\Data\CNCDATA\RED_ALERT\CD1\`; `COUNTERSTRIKE\` also has `SIZZLE*.VQA`). Never commit it, redistribute it, or download replacements/assets. See [agent_policies/licensing.md](agent_policies/licensing.md).
- **Do not download or install third-party executables** (DirectDraw shims like dgVoodoo2/dxwrapper, cracks, "fixed" DLLs; do not load the Steam install's `bink2w32.dll`). A previous attempt was flagged by the owner's antivirus and abandoned; the D3D11 presenter exists so none is needed. Don't suggest ways around the antivirus. Any new third-party *source* dependency (e.g. for movies) must be discussed with the owner first (license, build integration).
- **Fix the source, never weaken the platform**: the owner rejected making `.rdata` writable (`/SECTION:.rdata,RW`) - literal/const-data writes are fixed where they happen (#18). Same spirit: no suppressing a validation to hide a bug (`MapClass::Validate` was fixed properly, #23).
- **`REDALERT.INI` must be a private copy**, never a hard link (the game rewrites it; once modified the owner's Steam file). `scripts/runtest.ps1 setup` does it right.
- Commit messages: short imperative subject, then a body explaining *why*, ending with the `Co-Authored-By` trailer given in the session. Never force-push; upstream (`electronicarts/CnC_Red_Alert`) takes no PRs - `origin` is the owner's fork. Commit when the owner asks or at milestones (they said: update all docs, then commit, before any further work).
- Duplicate headers: `WIN32LIB/INCLUDE/X.H` and `WIN32LIB/<MODULE>/X.H` (and a few in `CODE/`) exist; the include order decides which compiles. Edit both, then `diff`. `SRCDEBUG/`, `OLD*/`, `TEST/`, `*.BAK` are dead copies.
- File-editing quirks: some sources are Latin-1 (the Edit tool refuses them; patch with a script, keep CRLF); a Python write once inserted NUL bytes; PowerShell here-strings fail with `git commit -F -` - use a Bash heredoc.

## 8. Working style that has paid off here
- **Verify by running; report real movement** ("the crash moved from X to Y", "mission 1 -> map now works"). "It links" was true for weeks while the rewritten blit code had never executed. The owner's eyes beat screenshots (PowerShell screenshots often capture the agent app, not the topmost game window).
- **Root-cause, don't patch symptoms.** Almost every crash was one of a handful of bug classes; after the first hit of a class, *search for the class* (grep, compiler warnings, a script like `size_enums.py`) instead of waiting for the next crash - the owner asked for exactly that.
- Keep per-crash notes in the docs as you go; the owner explicitly wants findings recorded.
- Environment: German-locale Windows 11, Intel iGPU, VS 2026 (MSBuild messages German; `error C####` tags are not). The machine once did a hardware "safety shutdown" during a test run (unrelated to the code as far as anyone can tell) - the script kills the game after a timeout, keep runs short.

## 9. Known unknowns / backlog
- **Sound effects** (top priority, see table). Also unverified: speech/EVA, `Sample_Copy` for the other compression types.
- **Movies**: [plans_wip/movies.md](plans_wip/movies.md) (decision pending; owner's idea: offline-transcode their own `.BK2` files to AV1 into a git-ignored cache + play them with a permissive decoder such as dav1d).
- Mission 2+, save/load (structs with 32-bit pointers not audited), skirmish, in-game dialogs, options menu, alt-tab/device loss, window resize.
- Audits not done yet (see [Analysis/summary.md](Analysis/summary.md)): WIN32LIB on-disk structs against 2-byte packing line by line; enums that `scripts/size_enums.py` skips (`KeyNumType`, `VersionEnum`, `MapEnum`) and enums inside persisted structs; ~157 harmless-looking literal -> `char *` bindings; trigger-name pointer in a 32-bit slot (`TACTION.CPP:267`, `TRIGTYPE.CPP:1895`).
- `COORD.CPP` `calcx`/`calcy` and the `PREDATOR` negative-offset table in `Buffer_Frame_To_Page` were flagged as not bit-exact-verified in 1c; `LCW_Comp`, `Build_Fading_Table`, the 2x interpolators partly exercised only (pitfalls #0).
- Unpaced loops other than `Sync_Delay` (menus, `Call_Back` spinners) only burn CPU; pace them with `Pace_To_Display()` if something depends on their rate.
- `IndexClass`-style template ODR bugs could exist for other packed/unpacked mismatches; the `static_assert(sizeof)` habit is the cheap defence.
