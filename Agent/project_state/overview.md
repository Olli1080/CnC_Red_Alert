---
name: repo-overview
description: What this repository is, its top-level layout, and file/extension breakdown
metadata:
  type: project_state
---

# Repository Overview

Electronic Arts' 2025 open-source release of the source code for **Command & Conquer: Red Alert** (Westwood Studios, 1996). See the root [README.md](../../README.md) and [LICENSE.md](../../LICENSE.md), and [agent_policies/licensing.md](../agent_policies/licensing.md) for what the license actually permits.

## What it is, architecturally
A single monolithic Win32/DOS executable game engine, written primarily in **Watcom C++** with hand-written **x86 assembly (TASM)** for performance-critical routines, spanning two library generations: an earlier DOS-extender ("DOS4GW") support library and its ported Win32-native successor, reflecting Westwood's mid-1990s DOS→Windows transition. The game engine itself (`CODE/`) sits on top of these support libraries.

## Top-level layout (9 modules, ~2,900 files)

| Directory | Files | Role | Docs |
|---|---:|---|---|
| `CODE/` | 589 | Main game engine: object classes, map/UI, AI/scripting, networking, save/load, compression, audio glue, entry point | [modules/code/summary.md](modules/code/summary.md) |
| `WIN32LIB/` | 568 | Westwood's Win32-native low-level support library (graphics, audio, input, fonts, comms) | [modules/win32lib.md](modules/win32lib.md) |
| `WWFLAT32/` | 400 | Predecessor DOS-extender ("flat 32-bit") support library that WIN32LIB was ported from | [modules/wwflat32.md](modules/wwflat32.md) |
| `VQ/` | 183 | VQA video codec libraries (engine-integrated copy) | [modules/vq_winvq.md](modules/vq_winvq.md) |
| `WINVQ/` | 216 | VQA codec libraries + standalone tools (viewer, player) — SDK-style packaging of the same codec | [modules/vq_winvq.md](modules/vq_winvq.md) |
| `IPX/` | 52 | Novell IPX/SPX LAN transport — 16↔32-bit thunk DLLs | [modules/ipx_launch_launcher_tools.md](modules/ipx_launch_launcher_tools.md) |
| `LAUNCHER/` | 47 | Win32 patch-and-run game launcher (MSVC6) | [modules/ipx_launch_launcher_tools.md](modules/ipx_launch_launcher_tools.md) |
| `TOOLS/` | 25 | Developer asset-authoring tools (`.MIX` packer, `.AUD` encoder, + 8 source-less binaries) | [modules/ipx_launch_launcher_tools.md](modules/ipx_launch_launcher_tools.md) |
| `LAUNCH/` | 2 | Minimal DOS pre-launch stub (memory/disk check, DOS4GW env setup) | [modules/ipx_launch_launcher_tools.md](modules/ipx_launch_launcher_tools.md) |

## File extension breakdown (whole repo)
`.H` (660), `.CPP` (610), `.ASM` (357), `.BAK` (83, stale duplicates — see [knowledge_base/codebase_history_and_cruft.md](../knowledge_base/codebase_history_and_cruft.md)), `.INC` (70, TASM includes), `.BOR`/`.WAT`/`.CFG` (Borland/Watcom build configs), `.EXE`/`.LIB` (prebuilt binaries), `.IDE`/`.PJT`/`.DSP`/`.DSW` (IDE/editor project files — Borland, Codewright, MSVC6 respectively). Full toolchain breakdown in [knowledge_base/build_toolchains.md](../knowledge_base/build_toolchains.md).

## Build status
The original Watcom/Borland/MSVC6 toolchain sprawl does not build this (README's own statement, and the tooling itself is largely unobtainable — see [agent_policies/toolchain_and_editing_constraints.md](../agent_policies/toolchain_and_editing_constraints.md)). A modernized CMake + vcpkg build (MSVC 2026/x64, see [plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md)) now compiles both `WIN32LIB/` (sub-milestone 1a) and `CODE/` (sub-milestone 1b) with 0 errors, linking into static libraries (`win32lib.lib`, `redalert_code.lib`). No executable exists yet: `.ASM` files aren't wired into the build (deferred to Phase 2/sub-milestone 1c), and a handful of files with confirmed-absent proprietary dependencies (Greenleaf Comm Library, two missing headers) are excluded rather than linked.
