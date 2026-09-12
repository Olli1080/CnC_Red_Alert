---
name: win32lib-1a-port-log
description: Live progress log for sub-milestone 1a (getting WIN32LIB compiling standalone under MSVC2026/x64/CMake)
metadata:
  type: plan
---

# WIN32LIB Sub-milestone 1a — Port Log

Parent plan: [modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md), sub-milestone 1a. This file tracks exact progress so a future session can resume without re-diagnosing anything.

## How to reproduce
```
cmake --preset Main
cmake --build build/Main --config Release --target win32lib
```
`WIN32LIB/CMakeLists.txt` builds a curated, non-recursive-glob source list per live subdirectory (excludes `SRCDEBUG/`, `EXAMPLE/`, every nested `OLD/OLDTEST/TEST/OLDMEM/MSVC/UTIL/` subfolder, `MOVIE/` entirely, and `AUDIO/TST.CPP` specifically — see that file's comments for why). `.ASM` files are not yet in the target (deferred until UASM is wired in) — this only proves the C++ side.

## Progress
| Point | Error count | Files with errors |
|---|---:|---:|
| Initial full build attempt | 1650 | 25 |
| After the `WWSTD.H` root-cause fix + `KEYBOARD.CPP`/`KEYBOARD.H` fixes | 180 | 18 |
| After syncing the stale `MISC/WWSTD.H` duplicate | ~180 | 17 |
| After excluding `MOVIE/` and `AUDIO/TST.CPP`, fixing `ICONCACH.CPP` | **173** | **14** |

## Root-cause fix: `WIN32LIB/INCLUDE/WWSTD.H` (fixed, high-leverage)
This one file's bugs cascaded into ~1470 of the original 1650 errors. Two independent issues, both fixed:

1. **The `#include <windows.h>` guard was wrong.** The header used `#ifndef WIN32` as a proxy for "has `<windows.h>` already been included" — a pattern that assumed `WIN32` only ever got defined by a prior actual inclusion. Our CMake-generated MSBuild project predefines `WIN32` directly as a compiler flag (standard practice for any Win32-platform project), so the guard now always thinks windows.h is already present and skips including it — leaving `UINT`/`BOOL`/etc. genuinely undefined when later headers need them. **Fix: check `_WINDOWS_`** (windows.h's own real include guard) instead, which is correct regardless of how/why `WIN32` got defined. This is the fix that dropped 1650 → 180.
2. **Unconditional `#include <dos.h>`/`<bios.h>`** — DOS-era headers that don't exist under MSVC at all. Now guarded behind `#if !defined(_WIN32) && !defined(WIN32)` (never true for our target, matches the file's own already-existing intent to special-case Win32).

Also converted `VOID`/`BOOL`/`INT`/`UINT`/`BYTE`/`UBYTE`/`UCHAR`/`WORD`/`UWORD`/`USHORT`/`LONG`/`ULONG`/`REALPTR`/`FARPTR` from blind `#define` macros to `typedef`s, per the project owner's suggestion — a macro replaces every later occurrence of the bare token anywhere in the translation unit (including inside Windows SDK headers included afterward), which is exactly the class of bug the `_WINDOWS_` guard above was fixing; a typedef is a real, scoped declaration the compiler can check and safely re-declare identically. All of these are now also gated behind `#ifndef _WINDOWS_` (they were only ever meant for a non-Windows target, and now never fire in our build at all — see below on `MISC/WWSTD.H`).

**Known duplicate**: `MISC/WWSTD.H` is a stale pre-fix copy of this same file (see [../knowledge_base/codebase_history_and_cruft.md](../knowledge_base/codebase_history_and_cruft.md)). Three files (`MISC/DELAY.CPP`, `MISC/FINDARGV.CPP`, `MISC/VERSION.CPP`) use `#include "wwstd.h"` (quotes), which checks the *local* directory first regardless of `-I` order — so putting `INCLUDE/` first in the include path (the general policy adopted for this target) does **not** help these three. Synced `MISC/WWSTD.H` to match `INCLUDE/WWSTD.H` exactly as a stopgap. **Follow-up worth doing**: audit the rest of the tree for other quote-included duplicate umbrella headers rather than fixing each stale copy reactively as it's discovered.

## Other fixes applied (small, individually diagnosed)
- **`KEYBOARD.CPP`**: two instances of the classic pre-standard "leaky for-loop scope" idiom (Watcom let a `for (short lp = ...)` loop variable stay visible in the enclosing block; standard C++ scopes it to the loop only) — added the missing re-declaration. Same pattern, same fix in **`DRAWBUFF/ICONCACH.CPP`**. **If this pattern recurs elsewhere, grep for a `for` variable used again in a later statement without re-declaring it.**
- **`KEYBOARD.CPP`**: two methods (`Put_Key_Message`, `Down`) were missing their return type entirely — old "implicit int" K&R-style default, illegal in modern C++. Added the return types from their header declarations (`BOOL`, `int` respectively). **If this pattern recurs, check the class's `.H` declaration for the real return type.**
- **`KEYBOARD.H`**: `CurrentCursor` was declared as generic `HANDLE` instead of `HCURSOR`; modern `winnt.h` uses `DECLARE_HANDLE`-based distinct opaque types so the implicit conversion `SetCursor(HANDLE)` no longer compiles. Fixed to the semantically-correct `HCURSOR`.
- **Batch fix (8 files)**: `#include <mem.h>` → `<string.h>`. `mem.h` is a pre-standard Borland/Turbo-C header for `memcpy`/`memset`/etc.; MSVC's CRT never shipped it. Files: `AUDIO/SOUNDIO.CPP`, `AUDIO/SOUNDLCK.CPP`, `IFF/LOADPICT.CPP`, `MEM/MEM.CPP`, `MONO/MONO.CPP`, `PALETTE/PALETTE.CPP`, `TILE/ICONSET.CPP`, `WSA/WSA.CPP`.
- **Excluded from the build** (not "fixed" — genuinely out of scope): `MOVIE/MOVIE.CPP` (needs classic DirectShow SDK baseclasses — `streams.h`, `amstream.h`, `ddstream.h`, `mmstream.h` — never in any modern Windows SDK; deferred to the Phase 4 Media Foundation rewrite per [../knowledge_base/modernization_options.md](../knowledge_base/modernization_options.md)). `AUDIO/TST.CPP` (a broken 3-line stub, `main {}` isn't valid C++ — dead test scaffolding, not real library code).

## Remaining files, triaged but not yet fixed (14 files, 173 errors)

| File | Symptom | Hypothesis |
|---|---|---|
| `AUDIO/SOUNDINT.CPP` | C4430 missing type specifier (implicit int) | Likely another missing-return-type case, same pattern as `KEYBOARD.CPP` — check its function definitions against `SOUNDINT.H`'s declarations. |
| `AUDIO/SOUNDIO.CPP`, `MEM/ALLOC.CPP` | C1083 can't open `bios.h` | These genuinely call DOS BIOS functions, not just include the header incidentally — need to check whether the calls are inside dead/unreachable branches for the Win32 build (likely, given `WIN32LIB` is supposed to be the Windows-native library) before deciding whether to guard-out the include or actually replace the calls. |
| `INCLUDE/function.h` | Multiple: `far`/`void`/`__cdecl` used in invalid old-style declarator syntax, `BOOL` unrecognized | Old Watcom-era forward-declaration syntax (register-based calling convention declarations like `_saveregs`) needs modernizing to standard declarator syntax. Not yet root-caused — the `BOOL` "unrecognized identifier" is suspicious given the `_WINDOWS_` guard work above; check this header's own include order relative to `WWSTD.H`/`windows.h`. |
| `INCLUDE/rawfile.h`, `RAWFILE/wwfile.h` | C2065 `_READ` undeclared | Expects a POSIX-style file-mode constant Watcom/DOS provided (via `<io.h>`/`<fcntl.h>`) under a different name than MSVC's CRT uses (`_O_RDONLY` etc.) — needs the actual constant name mapped. |
| `KEYBOARD/MOUSE.CPP` | C2664 `timeSetEvent` callback signature mismatch | The multimedia timer callback's parameter types don't exactly match modern `LPTIMECALLBACK` (`DWORD_PTR` vs `DWORD` — a 32→64-bit widening in the real API signature over the decades). Needs the callback's own signature updated to match, not just a cast. |
| `PLAYCD/REDBOOK.CPP` | Long cascade: "RedBookClass: no class or namespace", then dozens of "undeclared identifier" | Classic single-root-cause cascade (compare `KEYBOARD.CPP`'s original 114-error cascade from one bug). Hypothesis: `PLAYCD.H`'s class declaration itself fails to parse — the module's own docs note `PLAYCD.H` has DOS-era content "largely under a `NOT_FOR_WIN95` legacy `#ifdef`" (see [../project_state/modules/win32lib.md](../project_state/modules/win32lib.md)); worth checking whether that guard is inverted/stale in the same spirit as the `WWSTD.H` `WIN32` bug, or whether it references `SEGSEL`/DPMI types that no longer exist for the Win32 path. **Investigate this header first** — likely another single high-leverage fix. |
| `PROFILE/PROFILE.CPP`, `PROFILE/WPROFILE.CPP`, `TIMER/TIMERINI.CPP`, `RAWFILE/CCFILE.CPP`, `RAWFILE/RAWFILE.CPP` | Not yet individually triaged | Next files to investigate. |

## Next steps
1. Investigate `PLAYCD/REDBOOK.CPP` first — likely the next highest-leverage single fix given the cascade shape.
2. Work through the remaining triaged files above.
3. Once `WIN32LIB` compiles clean (0 errors), the next question is `.ASM` files: either wire in UASM (per [modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md) Phase 2) or confirm the static-library-with-missing-symbols approach is fine until link time.
4. Then move to sub-milestone 1b (`CODE/` compiling against this).
