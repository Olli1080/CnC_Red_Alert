---
name: toolchain-and-editing-constraints
description: The original Watcom/TASM toolchain is unobtainable but a modern CMake/MSVC x64 build works; legacy dependencies, and how to handle backup/binary/duplicate files safely
metadata:
  type: agent_policy
---

# Toolchain & Editing Constraints

## Build status (updated 2026-09-20)
Per the root [README.md](../../README.md), the *original* source "does not fully compile" with its original tools, and those tools (Watcom 10.6, TASM 4.0, DirectX 5 SDK, GCL, HMI SOS) are largely unobtainable. **That is no longer the practical situation:** the owner's modernization effort (CMake + vcpkg submodule, MSVC 2026, **x64 only**) builds `win32lib.lib`, `redalert_code.lib` and a linking `redalert.exe` with 0 errors and 0 unresolved externals - see [../HANDOFF.md](../HANDOFF.md) for the exact state and [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md) for the plan. Build with `cmake --preset Main` then `cmake --build build/Main --target redalert --config Debug -- -m` (or `scripts/runtest.ps1 build`).

Consequences for agents:
- **"Does it compile" IS now a usable signal - but a weak one.** It says nothing about runtime correctness (most of the ~66 `.ASM` files rewritten as C++ had never executed when this was written). Verify by running: [../knowledge_base/debugging_and_runtime_testing.md](../knowledge_base/debugging_and_runtime_testing.md).
- The legacy per-module makefiles (`MAKEFILE`, `.BOR`, `.WAT`, `.IDE`, `.DSP`) are still **reference material only**; the CMake files are the live build. They are, however, the authoritative record of the *original* compiler flags - e.g. `CODE/MAKEFILE:113` `/zp1` (byte-packed structs), which turned out to matter enormously ([../knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md)).
- Source lists: `CODE/CMakeLists.txt` is an explicit list transcribed from Westwood's own `MAKEFILE`; `WIN32LIB/CMakeLists.txt` globs the live module directories and excludes `SRCDEBUG/`, `OLD*/`, `TEST/`, `MSVC/`, `UTIL/`.
- Running the game requires the owner's own game data (Steam CnC Remastered Collection). Never add, download or commit game assets.

## Original toolchain requirements (per README - historical; not needed for the CMake build)
- Watcom C/C++ v10.6 (the primary/shipped compiler for `CODE/`, `WIN32LIB/`, `WWFLAT32/`, `VQ/`, `WINVQ/`)
- Borland Turbo Assembler (TASM v4.0) for `.ASM` files

## Missing third-party dependencies (per README)
None of these are vendored in the repo; you cannot "just build" without sourcing them first:
- DirectX 5 SDK
- DirectX Media 5.1 SDK
- Greenleaf Communications Library (GCL)
- Human Machine Interface (HMI) "Sound Operating System" (SOS) — note: some SOS **headers** and a few prebuilt `.LIB` binaries *are* vendored (e.g. under `WIN32LIB/AUDIO/`, `VQ/LIB` docs + `WINVQ/LIB/*.LIB`), but the full SDK/driver source is not.

To use compiled binaries at all, the README notes you must own the game (available via EA App or Steam).

## Editing caveats specific to this legacy tree
- **`.BAK` files are everywhere** (dozens across `CODE/`, `WIN32LIB/`, `WWFLAT32/`, `VQ/`/`WINVQ/`, `IPX/`, `LAUNCHER/`) — stale duplicate snapshots from old internal VCS systems, not active source. Never treat a `.BAK` file as the file to edit; don't delete them either unless the user explicitly asks for cleanup (some, like `CODE/CONQUER.CPP.BAK`, are historically interesting diffs against the live file — see [../knowledge_base/codebase_history_and_cruft.md](../knowledge_base/codebase_history_and_cruft.md)).
- **Whole superseded module-copy subdirectories exist** (e.g. `WIN32LIB/KEYBOARD/OLD/`, `WIN32LIB/MEM/OLDMEM/` and `MSVC/`, `WWFLAT32/AUDIO/NEW/`, `WINVQ/VQA32/OLD/`, `IPX/OK/` and `IPX/OLD/`) — near-duplicates of their parent module's active files. Don't edit these when the intent is to change current behavior; confirm which copy is "live" first (usually the one directly in the module's top-level directory, not in a nested `OLD`/`NEW`/`OK`/alternate-compiler subfolder — but `IPX/OK/` is the inverse case, a verified-good *reference* snapshot, not dead code — see [../project_state/modules/ipx_launch_launcher_tools.md](../project_state/modules/ipx_launch_launcher_tools.md)).
- **Binary files masquerading among text source**: `.IDE` (Borland/Watcom IDE project files), `.OBJ`/`.EXE`/`.LIB` (prebuilt binaries, e.g. `TOOLS/*.EXE`, `WINVQ/VQAVIEW/VQAVIEW.EXE`, `WINVQ/LIB/*.LIB`) are not text — don't try to read or diff them as such.
- **Multiple coexisting build systems per module, not equally live**: e.g. in `CODE/`, the extensionless `MAKEFILE` (Watcom) + `MAKE_ALL.BAT` is the current path; `TURBOC.CFG`/`TLINK.CFG`/`.PJT`/`.IDE` files are legacy/historical artifacts from earlier in the project's life. In `WIN32LIB/`/`WWFLAT32/`, the plain `MAKEFILE` = Watcom (primary), `MAKEFILE.BOR` = Borland (secondary), `MAKEFILE.WAT` = explicit alternate Watcom variant. `LAUNCHER/` is the one module that's genuinely MSVC6-native (`LAUNCHER.DSP`/`.DSW`), not Watcom. Don't assume changing one build file updates all of them — they're often stale copies of each other.
- **Compile *and run* your changes** (see Build status above). Cross-reference [../project_state/](../project_state/summary.md) for how a subsystem is supposed to work; for the legacy ASM that was rewritten in C++, the original `.ASM` is still next to the rewrite and is the spec.
- **Hardcoded developer-machine paths** appear throughout build/config files (e.g. `D:\BC45\INCLUDE`, `F:\projects\c&c0\vcs\code`, `C:\rawolapi\SOURCE\WATCOM`) — these are historical artifacts of Westwood's original build machines, not something to "fix"; they indicate which build files are legacy vs. still relevant to the environment variables the current `MAKEFILE`s expect (`WATCOM`, `WWFLAT`, `WWVCS`, `WIN32LIB`, `BLADE`).
