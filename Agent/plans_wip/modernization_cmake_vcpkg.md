---
name: modernization-cmake-vcpkg
description: Active plan to rebuild this codebase's build system on CMake + vcpkg (submodule) targeting modern MSVC/Windows APIs
metadata:
  type: plan
---

# Modernization Plan: CMake + vcpkg

**Status: planning stage — research complete, no code/build changes made yet.**

Owner-stated direction (2026-09-12): move to modern CMake for build configuration, and vcpkg — added as a git submodule — as the dependency installer, as far as possible. Background research behind the specific replacement choices below: [../knowledge_base/modernization_options.md](../knowledge_base/modernization_options.md).

## Goal
Replace the current per-module Watcom/Borland/MSVC6 makefile-and-IDE-project sprawl (documented in [../knowledge_base/build_toolchains.md](../knowledge_base/build_toolchains.md)) with a single top-level CMake build, using vcpkg in manifest mode for third-party dependencies, targeting current MSVC (Visual Studio 2026) and modern Windows APIs. This is a **modernization**, not a restoration of the original 1996 build — see [../agent_policies/toolchain_and_editing_constraints.md](../agent_policies/toolchain_and_editing_constraints.md) for why the original toolchain can't currently build the code either way.

## Scope decision: which modules get carried forward
Based on [../project_state/overview.md](../project_state/overview.md) and [../knowledge_base/codebase_history_and_cruft.md](../knowledge_base/codebase_history_and_cruft.md):
- **In scope**: `CODE/` (game logic — needs platform-layer edits, not a full rewrite), `WIN32LIB/` (the Windows-targeted support library — the actual base to build forward from), one of `VQ/`/`WINVQ/` (video codec — open decision #4 in modernization_options.md).
- **Excluded entirely**: `WWFLAT32/` (DOS-extender predecessor already superseded by `WIN32LIB/` for the Windows build — carrying it forward would mean porting real-mode/DPMI code that has to be deleted anyway), `IPX/` (16-bit thunk DLLs cannot run on 64-bit Windows at all — replace with the existing `TCPIP.CPP`/`WSPUDP.*` socket path instead), `LAUNCH/` (DOS pre-launch memory/disk-space stub — meaningless on modern Windows).
- **Deferred, not blocking**: `LAUNCHER/` (Win32 patch launcher — small, self-contained, can move to CMake independently later), `TOOLS/` (dev-only asset tools — only needed if rebuilding/modifying game asset files, not for running the game).

## Phased approach

### Phase 0 — Repo/build skeleton
- Add `vcpkg` as a git submodule at the repo root (`vcpkg/`), with a `vcpkg.json` manifest declaring dependencies as they're identified. Expected candidates: `directx-headers` (and possibly `directx-agility-sdk`), `sdl2` or `openal-soft` *only if* the cross-platform/OpenAL route is chosen over miniaudio/XAudio2 in Phase 1 — miniaudio itself is more likely vendored directly as a single header than pulled via vcpkg, since that's its intended usage model.
- Root `CMakeLists.txt` wiring the vcpkg toolchain file (`vcpkg/scripts/buildsystems/vcpkg.cmake`) and a `CMakePresets.json` targeting the VS2026/MSVC generator.
- No source changes in this phase — it only needs to prove the build skeleton configures cleanly (an empty/stub target is fine).

### Phase 1 — Platform-layer replacement (the actual hard part)
**Blocked on resolving the four open decisions in [../knowledge_base/modernization_options.md](../knowledge_base/modernization_options.md)** (ASM vs. C++ rewrite, x86 vs. x64, rendering-layer target, VQ vs. WINVQ base) — each significantly changes how much of `WIN32LIB/` gets rewritten vs. wrapped, so don't start writing code here until those are picked.
- Replace `WIN32LIB/MISC/DDRAW.CPP` per the chosen rendering strategy (compatibility shim, or native D3D11/D3D12 rewrite).
- Replace `WIN32LIB/AUDIO/SOS*` with miniaudio or XAudio2.
- Replace `WIN32LIB/MOVIE/`'s DirectShow calls with Media Foundation, only if that FMV path is still wanted (the primary cutscene path is the unrelated custom VQA codec).
- Drop `WINCOMM`'s legacy `NOT_FOR_WIN95`-guarded paths and the null-modem multiplayer mode entirely (`NULLCONN`, `MPMGRD`).

### Phase 2 — Toolchain migration for what's kept
- If ASM is kept (decision #1): convert the retained `.ASM` files to UASM-assembled objects wired into CMake via a custom command/target.
- Get `WIN32LIB/` compiling clean under MSVC 2026 as a static library target in the new CMake build.
- Get `CODE/` compiling against it.

### Phase 3 — Verify
- Confirm the resulting executable links and runs against real game assets (per [../agent_policies/licensing.md](../agent_policies/licensing.md), the user needs to own the game for the assets — this plan only ever touches code).
- Update [../project_state/overview.md](../project_state/overview.md)'s "Build status: does not currently compile" line once genuinely false.
- Move this file to [../plans_done/summary.md](../plans_done/summary.md) once the new build is the working baseline.

## Status log
- **2026-09-12**: Plan created; research phase complete (toolchain/dependency replacements identified in modernization_options.md). No code or build-file changes made yet. The four open engineering decisions above block the start of Phase 1 — need the project owner's call on each before proceeding.
