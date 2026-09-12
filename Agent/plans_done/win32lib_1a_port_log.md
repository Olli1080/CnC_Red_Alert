---
name: win32lib-1a-port-log
description: Completed progress log for sub-milestone 1a (WIN32LIB now compiles standalone under MSVC2026/x64/CMake, 0 errors)
metadata:
  type: plan
---

# WIN32LIB Sub-milestone 1a — Port Log (COMPLETE)

Parent plan: [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md). **`WIN32LIB` now builds with 0 errors** (`win32lib.lib` produced) — verified via a real `cmake --build`, not just authored. Started at 1650 errors across 25 files.

## How to reproduce
```
cmake --preset Main
cmake --build build/Main --config Release --target win32lib
```
`WIN32LIB/CMakeLists.txt` builds a curated, non-recursive-glob source list per live subdirectory (excludes `SRCDEBUG/`, `EXAMPLE/`, every nested `OLD/OLDTEST/TEST/OLDMEM/MSVC/UTIL/` subfolder, plus specific files documented in that CMakeLists.txt as genuinely out of scope — see "Files excluded" below). `.ASM` files are not yet in the target (deferred to Phase 2 — UASM wiring) — this proves only the C++ side, and the resulting static library can still have unresolved ASM symbols until link time against a real executable.

## Progress
| Point | Error count | Files with errors |
|---|---:|---:|
| Initial full build attempt | 1650 | 25 |
| After the `WWSTD.H` root-cause fix + `KEYBOARD.CPP`/`KEYBOARD.H` fixes | 180 | 18 |
| After syncing the stale `MISC/WWSTD.H` duplicate | ~180 | 17 |
| After excluding `MOVIE/` and `AUDIO/TST.CPP`, fixing `ICONCACH.CPP` | 173 | 14 |
| After excluding `PLAYCD/REDBOOK.CPP` (dead `NOT_FOR_WIN95`-only class) | 49 | 13 |
| After excluding `RAWFILE/CCFILE.CPP` (missing header) and `PROFILE/PROFILE.CPP` (superseded duplicate) | 11 | 5 |
| After the `_READ`/`_WRITE`, `function.h`, and `DebugBuffer` fixes | 4 | 1 (`SOUNDIO.CPP`) |
| **Final** | **0** | **0** |

## Root-cause fix #1: `WIN32LIB/INCLUDE/WWSTD.H` — the `_WINDOWS_` include-guard bug
This one file's bugs cascaded into ~1470 of the original 1650 errors.

1. **The `#include <windows.h>` guard was wrong.** The header used `#ifndef WIN32` as a proxy for "has `<windows.h>` already been included" — a pattern that assumed `WIN32` only ever got defined by a prior actual inclusion. Our CMake-generated MSBuild project predefines `WIN32` directly as a compiler flag (standard practice for any Win32-platform project), so the guard always thought windows.h was already present and skipped it, leaving `UINT`/`BOOL`/etc. genuinely undefined when later headers needed them. **Fixed to check `_WINDOWS_`** (windows.h's own real include guard) instead, which is correct regardless of how/why `WIN32` got defined. This single fix dropped 1650 → 180.
2. **Unconditional `#include <dos.h>`/`<bios.h>`** — DOS-era headers that don't exist under MSVC. Guarded behind `#if !defined(_WIN32) && !defined(WIN32)`.

Also converted `VOID`/`BOOL`/`INT`/`UINT`/`BYTE`/`UBYTE`/`UCHAR`/`WORD`/`UWORD`/`USHORT`/`LONG`/`ULONG`/`REALPTR`/`FARPTR` from blind `#define` macros to `typedef`s (owner's suggestion) — a macro replaces every later occurrence of the bare token anywhere in the translation unit (including inside Windows SDK headers included afterward), exactly the class of bug the `_WINDOWS_` guard was fixing; a typedef is a real, scoped declaration the compiler can check and safely re-declare identically. All gated behind `#ifndef _WINDOWS_`.

**Known duplicate handled**: `MISC/WWSTD.H` was a stale pre-fix copy (see [../knowledge_base/codebase_history_and_cruft.md](../knowledge_base/codebase_history_and_cruft.md)). Three files (`MISC/DELAY.CPP`, `MISC/FINDARGV.CPP`, `MISC/VERSION.CPP`) use `#include "wwstd.h"` (quotes), which checks the *local* directory first regardless of `-I` order — the general "`INCLUDE/` first" policy doesn't help these. Synced `MISC/WWSTD.H` to match exactly. **Follow-up worth doing later**: audit the tree more broadly for other quote-included duplicate umbrella headers rather than fixing each reactively.

## Root-cause fix #2: `#pragma pack(N)` before `<windows.h>`'s first inclusion
Found in both `AUDIO/SOUNDIO.CPP` and `AUDIO/SOUNDINT.CPP`: each set `#pragma pack(4)` (a Watcom-era convention for tight binary-format structs) *before* their own first `#include <windows.h>`. Modern Windows SDK headers `static_assert(__alignof(LARGE_INTEGER) == 8, ...)` to catch exactly this — non-default packing corrupts SDK struct layouts. This bug was always latent but only surfaced once fix #1 made `WWSTD.H` actually include windows.h correctly (previously the buggy guard was accidentally "protecting" against it by skipping the inclusion entirely). **Fixed** by moving `<windows.h>`/`<windowsx.h>`/`"dsound.h"` to before any `#pragma pack` line in both files — later guarded re-inclusions elsewhere in the same file (e.g. via `audio.h` → `wwstd.h`) are harmless no-ops. **If this pattern recurs, check `#pragma pack` position relative to each file's first windows.h include** — a quick scan of the rest of the live tree found no other instances.

## Other fixes applied (small, individually diagnosed)
- **Leaky pre-standard for-loop scope** (Watcom let a `for (short lp = ...)` variable stay visible in the enclosing block; standard C++ scopes it to the loop only): `KEYBOARD.CPP` (×2), `DRAWBUFF/ICONCACH.CPP`, `AUDIO/SOUNDIO.CPP` (`index`, fixed by hoisting the declaration above the loop instead of re-declaring, since it's used after the loop).
- **Missing return types** (old K&R "implicit int" default, illegal in modern C++): `KEYBOARD.CPP`'s `Put_Key_Message`→`BOOL`, `Down`→`int` (from the header declarations).
- **Over-generic `HANDLE`**: `KEYBOARD.H`'s `CurrentCursor` was `HANDLE`, needed `HCURSOR` — modern `winnt.h` uses `DECLARE_HANDLE`-based distinct opaque types so the implicit conversion into `SetCursor()` no longer compiles.
- **Batch fix (8 files)**: `#include <mem.h>` → `<string.h>` (pre-standard Borland/Turbo-C header MSVC's CRT never shipped): `AUDIO/SOUNDIO.CPP`, `AUDIO/SOUNDLCK.CPP`, `IFF/LOADPICT.CPP`, `MEM/MEM.CPP`, `MONO/MONO.CPP`, `PALETTE/PALETTE.CPP`, `TILE/ICONSET.CPP`, `WSA/WSA.CPP`.
- **`_READ`/`_WRITE` never defined anywhere**: `wwfile.h`'s `#define READ _READ` expected these from the old Watcom runtime, which MSVC's CRT never provided under these names. Added `#define _READ 1` / `#define _WRITE 2` (matching the bitwise-OR usage pattern, `case READ|WRITE:`, in `RAWFILE.CPP`). Fixed in both `INCLUDE/wwfile.h` and its synced duplicate `RAWFILE/wwfile.h`.
- **`INCLUDE/function.h`**: `_saveregs`/`_loadds`/`far` are Watcom 16-bit real-mode calling-convention/segment-attribute keywords, meaningless (and unrecognized) under Win32/x64 — stripped from the `Sound_Callback`/`maintenance_callback`/`VQA_TimerCallback` prototypes and from `maintenance_callback`'s actual definition in `SOUNDINT.CPP`, which used the same qualifiers.
- **Dead `extern DebugBuffer[];`** in `SOUNDINT.CPP` — missing type (old implicit-int) and never defined or used anywhere in the tree. Removed rather than guessing a type for something never referenced.
- **`timeSetEvent` callback signature drift**: modern `LPTIMECALLBACK` takes `DWORD_PTR` for its last three parameters (widened from `DWORD` as part of the general Win64 pointer-sized-parameter convention). Fixed the declared signatures (removing now-unnecessary casts, some of which were genuinely invalid syntax — casting to a function *type* rather than a function *pointer* type) in: `PROFILE.CPP`'s `Profiler_Callback` (before that whole file was excluded, see below — left fixed anyway since it's harmless), `WPROFILE.CPP`'s `Thread_Callback`, `TIMER/TIMERINI.CPP`'s `Timer_Callback`, `AUDIO/SOUNDIO.CPP`'s `Sound_Timer_Callback`.
- **`WPROFILE.CPP`**: `CONTEXT::Eip` doesn't exist on x64 (that's the x86 32-bit instruction-pointer field name; x64's equivalent is `Rip`). Fixed with a note that the target (`ProfileFunctionAddress`) is still a 32-bit `unsigned`, so this truncates on x64 — acceptable for a dev-only profiling tool not to be perfectly correct yet, flagged for later if the profiler is ever actually used on x64.

## Files excluded from the build (genuinely out of scope, not "unfixed")
- **`MOVIE/MOVIE.CPP`** — needs classic DirectShow SDK baseclasses (`streams.h`, `amstream.h`, `ddstream.h`, `mmstream.h`) never in any modern Windows SDK. Deferred to the Phase 4 Media Foundation rewrite per [../knowledge_base/modernization_options.md](../knowledge_base/modernization_options.md).
- **`AUDIO/TST.CPP`** — a broken 3-line stub (`main {}` isn't valid C++). Dead test scaffolding.
- **`PLAYCD/REDBOOK.CPP`** — implements methods of `RedBookClass`, but that class is declared *only* inside `PLAYCD.H`'s `#ifdef NOT_FOR_WIN95` block (DOS real-mode DPMI code: `SEGSEL`, `DPMI_real_alloc`, `union REGS`) — a macro this Win32 build never defines. Nothing else references `RedBookClass`; this file was never actually part of the live Win32 library.
- **`RAWFILE/CCFILE.CPP`** — `#include`s `"ccfile.h"`, which genuinely doesn't exist anywhere in this GPL release (checked `SRCDEBUG/` and every `OLD/` copy too). A real gap in the source drop. `CODE/` has its own separate `CCFILE.H`/`CCFileClass` (see [../project_state/modules/code/file_io_compression_saveload.md](../project_state/modules/code/file_io_compression_saveload.md)) but its member layout hasn't been verified compatible — **revisit this when sub-milestone 1b (`CODE/` integration) clarifies actual `CCFileClass` usage.**
- **`PROFILE/PROFILE.CPP`** — proven to be a stale/superseded duplicate of `WPROFILE.CPP`: both define identical `Start_Profiler(void)`/`Stop_Profiler(void)` (a genuine ODR violation if both were ever linked). `WPROFILE.CPP`'s callbacks (`Old_Profiler_Callback`/`New_Profiler_Callback`) match real ASM-defined symbols in `APROFILE.ASM`; `PROFILE.CPP`'s own callback and its `Copy_CHK()` call match nothing anywhere in the tree (not even a stale comment reference resolves to a real symbol). `WPROFILE.CPP` is the real, live implementation.

## Next steps (sub-milestone 1b and beyond)
1. `.ASM` files: wire in UASM per [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md) Phase 2, or confirm the current ASM-free static library is sufficient until actual link time against `CODE/`.
2. Move to sub-milestone 1b: get `CODE/` compiling and linking against this `win32lib`. This will surface whether `CCFileClass` (see above) needs reconstructing.
3. Clean up the 19 remaining warnings if/when convenient (unrecognized Borland pragmas, a couple of pointer truncation warnings in `SOUNDIO.CPP`) — none block the build.
