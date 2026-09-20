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
The original Watcom/Borland/MSVC6 toolchain sprawl does not build this (README's own statement, and the tooling itself is largely unobtainable — see [agent_policies/toolchain_and_editing_constraints.md](../agent_policies/toolchain_and_editing_constraints.md)). A modernized CMake + vcpkg build (MSVC 2026/x64, see [plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md)) now produces a **linking, bootable `redalert.exe`** (0 unresolved externals), via sub-milestones 1a (`win32lib.lib`), 1b (`redalert_code.lib`), and 1c (the executable itself). No `.ASM` files remain in the build at all — the ~66 that were still needed were rewritten as C++ rather than assembled, since none of them can target this project's x64-only decision as 32-bit-flat assembly. A handful of files with confirmed-absent proprietary dependencies (Greenleaf Comm Library, two missing headers) or genuinely dead functionality (VQA/MPEG movie playback, modem/WOL multiplayer) remain excluded/stubbed rather than linked.

Run against the owner's own legally-owned Red Alert data, the exe boots through window creation, INI parsing, and MIX-file discovery, then past DirectDraw/rendering init — the original 8-bit exclusive-fullscreen wall (a real WDDM driver limitation) turned out to be a total refusal of any 8-bit DirectDraw surface at all, worked around with a small Direct3D 11 presenter (8-bit index texture + 256-entry palette lookup texture + a tiny pixel shader) instead of a hardware DirectDraw primary; see [plans_done/code_1c_port_log.md](../plans_done/code_1c_port_log.md) for the fix and two deeper bugs it exposed (a wrong pixel-pointer formula and a 32-bit pointer truncation, both in this sub-milestone's own rewritten blit code, neither previously exercised since nothing ever got this far before). Boot now gets through INI/RSA-key init and MIX-archive parsing (both had struct-packing bugs — Westwood built `CODE/` with `/zp1`, MSVC doesn't; see [knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md)) and currently crashes in `Init_Mouse` (predicted: `.SHP` header packing). Nothing has been drawn yet; current state and next steps: [HANDOFF.md](../HANDOFF.md). Phase 4 (replacing the whole 8-bit blit pipeline with a modern SDL3-based renderer) remains the long-term target; the D3D11 presenter is a deliberately minimal stopgap (no window-resize or device-loss handling).
