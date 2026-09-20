---
name: build-toolchains
description: Which build system/toolchain is authoritative per module, and how the makefiles select platform/language
metadata:
  type: knowledge_base
---

# Build Toolchains

> **This file documents the ORIGINAL (1996) build systems.** The live build is now CMake + vcpkg on MSVC 2026 x64 (`CMakeLists.txt`, `CMakePresets.json`, `CODE/CMakeLists.txt`, `WIN32LIB/CMakeLists.txt`) - start at [../HANDOFF.md](../HANDOFF.md). The original makefiles remain useful as the record of the original compiler *flags* (notably `/zp1` byte-packing in `CODE/MAKEFILE:113`, see [pragma_pack_leak_odr.md](pragma_pack_leak_odr.md)) and of the authoritative source/object lists.

This repo accreted over several years (mid-90s) across multiple engineers and machines, so **several build systems coexist per module** and are not equally live. See [../agent_policies/toolchain_and_editing_constraints.md](../agent_policies/toolchain_and_editing_constraints.md) for the "don't assume any given build file works" caveat; this file records the verified specifics.

## Per-module toolchain summary

| Module | Primary (shipped) toolchain | Secondary/legacy artifacts present |
|---|---|---|
| [CODE/](../project_state/modules/code/summary.md) | Watcom (`wpp386`, `wlib`) via extensionless `MAKEFILE` + `MAKE_ALL.BAT` | Borland/Turbo C++ (`TURBOC.CFG`, `TLINK.CFG`), Codewright IDE (`.PJT`), Borland IDE (`.IDE`, binary), PC-Lint configs (`.LNT`) |
| [WIN32LIB/](../project_state/modules/win32lib.md) | Watcom, per-directory extensionless `MAKEFILE`, root `PROJECT.CFG`/`MAKE_LIB.MAK`/`MAKE_EXE.MAK` | Borland (`MAKEFILE.BOR` + `.IDE` per directory), explicit alt-Watcom (`MAKEFILE.WAT`) |
| [WWFLAT32/](../project_state/modules/wwflat32.md) | Watcom only — no `.BOR` files anywhere in this tree | Codewright (`.PJT` in `MCGAPRIM/`) |
| [VQ/](../project_state/modules/vq_winvq.md), [WINVQ/](../project_state/modules/vq_winvq.md) | Triple: generic `MAKEFILE`, `MAKEFILE.WAT` (Watcom), `MAKEFILE.BOR` + `BCC32.CFG`/`TASM32.CFG`/`TLIB.CFG`/`TLINK32.CFG` (Borland+TASM, PharLap TNT DOS-extender target) | `WINVQ/VQAVIEW` additionally uses a Watcom `.IDE` |
| [IPX/](../project_state/modules/ipx_launch_launcher_tools.md) | Watcom (`wpp`/`wlink`/`wlib`) + Microsoft `THUNK.EXE` (16↔32-bit thunk compiler) + TASM | — |
| [LAUNCH/](../project_state/modules/ipx_launch_launcher_tools.md) | MASM 6 (per source comment; no makefile checked in) | — |
| [LAUNCHER/](../project_state/modules/ipx_launch_launcher_tools.md) | **MSVC 6** (`LAUNCHER.DSP`/`.DSW`) — the one module not on Watcom/Borland | — |
| [TOOLS/AUDIOMAK/](../project_state/modules/ipx_launch_launcher_tools.md), [TOOLS/MIX/](../project_state/modules/ipx_launch_launcher_tools.md) | Borland C++ 4.x/4.5 + TASM (`MAKEFILE` generates `turboc.cfg`/`tasm.cfg`/`tlink.cfg`; `MIXFILE.IDE` binary Borland project) | — |

## How CODE/'s MAKEFILE selects platform & language
`CODE/MAKEFILE` (Watcom `wmake`, 755 lines) uses `!ifdef WIN32` / `!ifdef GERMAN` / `!ifdef FRENCH` macros. `MAKE_ALL.BAT` builds all three localized executables in one pass by invoking `wmake` with `WIN32=1`, `WIN32=1 GERMAN=1`, `WIN32=1 FRENCH=1` — this is why [CODE/ENG/](../project_state/modules/code/build_entrypoint_eng_wolapi.md) has `ENG/FRE/GER` string-table subfolders. `JOEMAKE.BAT` is an internal Westwood network/build-farm distribution script, not relevant to a normal local build.

## Required environment variables (per module's makefiles)
- `WATCOM` — Watcom install path (root `CODE/MAKE_ALL.BAT` hardcodes it to `C:\rawolapi\SOURCE\WATCOM` as a historical artifact — set your own).
- `WWFLAT`, `WWVCS` — required by `WWFLAT32/` makefiles.
- `WIN32LIB` — required by `WINVQ/VQAVIEW/MAKEFILE` (fails with `!error WIN32LIB Environment var not configured` if unset) and by the root `WIN32LIB/` build orchestration.
- `BLADE` — referenced by `WINVQ/VQAVIEW`'s library path (`$(BLADE)\VQA\LIB`) — a leftover from that tool's origin in Westwood's *Blade Runner* (1997) project (see [codebase_history_and_cruft.md](codebase_history_and_cruft.md)).

## Compiler flags of note
- `WIN32LIB/PROJECT.CFG` sets Watcom-style `/bt=nt` (NT target), `/otxan` (optimization flags).
- `WWFLAT32`'s `ASM_CFG` includes TASM `/jJUMPS` (optimize jumps) and `/ml` (case-sensitive linking) in `IDEAL` mode.
- `TASM.CFG` (in `CODE/`) uses `-mx -ml` flags.

## Practical guidance
Treat the **extensionless `MAKEFILE`** in each directory as the primary/most-current build path everywhere except `LAUNCHER/` (MSVC6-native). Everything else — `.BOR`, `.PJT`, `.IDE`, `TURBOC.CFG`/`TLINK.CFG` — is either a secondary supported toolchain (Borland, kept in parallel deliberately) or pure editor/IDE tooling cruft (Codewright `.PJT` files, binary `.IDE` project files) with no bearing on what actually ships.
