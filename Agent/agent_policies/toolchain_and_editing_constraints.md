---
name: toolchain-and-editing-constraints
description: The codebase does not build out of the box; required legacy toolchains, missing dependencies, and how to handle backup/binary files safely
metadata:
  type: agent_policy
---

# Toolchain & Editing Constraints

## The code does not currently build
Per the root [README.md](../../README.md): *"The current state of the source code does not fully compile and will require some effort to restore it."* Do not assume a `cmake`/modern-toolchain build is available or attempt to invent one implicitly — restoring buildability is itself a project the user would need to explicitly ask for. See [../knowledge_base/build_toolchains.md](../knowledge_base/build_toolchains.md) for the full, verified toolchain landscape (Watcom is primary; Borland/Turbo C++, Codewright, and MSVC6 artifacts coexist per-module and are **not** all equally authoritative).

## Required toolchains to actually compile (per README)
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
- **No compiler available to verify changes**: since nothing currently builds, "does it compile" isn't a usable correctness signal for now. Verify changes by reading/tracing the code (and cross-referencing [../project_state/](../project_state/summary.md) for how a subsystem is supposed to work) rather than assuming a build step will catch mistakes, unless the user has a working Watcom/TASM environment set up and asks you to use it.
- **Hardcoded developer-machine paths** appear throughout build/config files (e.g. `D:\BC45\INCLUDE`, `F:\projects\c&c0\vcs\code`, `C:\rawolapi\SOURCE\WATCOM`) — these are historical artifacts of Westwood's original build machines, not something to "fix"; they indicate which build files are legacy vs. still relevant to the environment variables the current `MAKEFILE`s expect (`WATCOM`, `WWFLAT`, `WWVCS`, `WIN32LIB`, `BLADE`).
