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

## Architecture decisions (resolved 2026-09-12 — see [../knowledge_base/modernization_options.md](../knowledge_base/modernization_options.md) for full reasoning)
1. **ASM**: kept for now (UASM-assembled); C++ rewrite of hot paths is deferred/backlog, not blocking.
2. **Target**: **x64 only** — x86/Win32 dropped entirely, not kept as a secondary target.
3. **Rendering — two horizons**: near-term = DirectDraw compatibility shim (dgVoodoo2 or [dxwrapper](https://github.com/elishacloud/dxwrapper)) to get the game running fastest; long-term (the actual "modernization + cross-platform" goal) = native rewrite on **SDL3** (windowing/input/audio/events) + **two separately hand-rolled backends, Vulkan and Direct3D 12** — explicitly **not** SDL3's `SDL_GPU` abstraction (owner's call: considered buggy/immature).
4. **Video codec base**: `VQ/VQA32`, not `WINVQ/VQA32` (identical core codec either way; `VQ/` is what `CODE/` already calls into and carries no unneeded tooling).
5. **Dear ImGui** for developer/debug tooling and likely the map-editor UI (not the in-game HUD, which keeps its original pixel-art widget rendering) — lands in the long-term rendering phase (needs a real Vulkan/D3D12 context), using its official `imgui_impl_sdl3` + `imgui_impl_vulkan`/`imgui_impl_dx12` backends, available via vcpkg's `imgui` port.

## Phased approach

### Phase 0 — Repo/build skeleton
Structure follows [../knowledge_base/cmake_vcpkg_reference_patterns.md](../knowledge_base/cmake_vcpkg_reference_patterns.md), the owner's established pattern from one of their other, more mature projects:
- Add `vcpkg` as a git submodule at **`externals/vcpkg`** (not a root-level `vcpkg/`), plus a root `vcpkg.json` manifest and a root `vcpkg-configuration.json` registering `overlay-ports`/`overlay-triplets` directories from day one (even if initially near-empty).
- Split build config into `cmake/Options.cmake` (options + `VCPKG_MANIFEST_FEATURES` toggling + compiler flags, **included before `project()`**), `cmake/FindDependencies.cmake` (all `find_package()` calls), and `cmake/Dependencies.cmake` (only if something ends up needed via `FetchContent` instead of vcpkg — expected to be empty/unused initially). Root `CMakeLists.txt` stays a thin orchestrator that just includes these in order.
- `vcpkg.json` dependencies expected for Phase 1-3: `directx-headers`. `miniaudio` gets wired via the header-only `find_path()` + `add_library(... INTERFACE IMPORTED)` workaround documented in the reference file (its vcpkg port ships no usable CMake config). SDL3/Vulkan/imgui are **not** needed until Phase 4.
- `CMakePresets.json` with a single `Main` preset: generator `"Visual Studio 18 2026"` (confirmed exact string from the reference project), `CMAKE_TOOLCHAIN_FILE` pointing at `./externals/vcpkg/scripts/buildsystems/vcpkg.cmake`, **x64 only** — no multi-arch preset matrix needed given the x64-only decision.
- No source changes in this phase — it only needs to prove the build skeleton configures cleanly (an empty/stub target is fine).

### Phase 1 — Get it running (compatibility-shim path, x64, minimal source changes)
Broken into sub-milestones (owner decision, 2026-09-12) rather than attempted as one push, starting with the library `CODE/` depends on before touching `CODE/` itself:
- **1a. `WIN32LIB/` compiles standalone as a static library** — ✅ **complete** (0 errors, `win32lib.lib` produced). Full record moved to [../plans_done/win32lib_1a_port_log.md](../plans_done/win32lib_1a_port_log.md).
- **1b. `CODE/` compiles and links against 1a's `WIN32LIB/`.** — ✅ **complete** (0 errors, `redalert_code.lib` produced). Full record moved to [../plans_done/code_1b_port_log.md](../plans_done/code_1b_port_log.md).
- **1c. The linked executable runs and reaches the original DirectDraw/audio/network call sites** — current focus. This is where the dxwrapper shim and the miniaudio swap actually get exercised for the first time, and where `.ASM` wiring (Phase 2) becomes unavoidable rather than deferrable.

Sub-milestone 1a is scoped further below.
This is the "immediate goal" milestone — get the existing engine running on modern x64 Windows with the smallest possible source footprint, deferring the real rendering/platform rewrite to Phase 4.
- Wrap the **existing, unmodified** `WIN32LIB/MISC/DDRAW.CPP` DirectDraw calls with dgVoodoo2 or dxwrapper — no rendering code changes yet.
- Replace `WIN32LIB/AUDIO/SOS*` with miniaudio or XAudio2 (unavoidable regardless of the rendering path — the original driver binary isn't available).
- Replace `VQ/`'s (not `WINVQ/`'s) audio/timer plumbing in `VQA32/AUDIO.CPP` to match whatever driver replaces SOS.
- Drop `WINCOMM`'s legacy `NOT_FOR_WIN95`-guarded paths and the null-modem multiplayer mode entirely (`NULLCONN`, `MPMGRD`).
- Drop `IPX/`'s 16-bit thunk DLLs and `CODE/IPX*.CPP`; multiplayer networking goes through the existing `CODE/TCPIP.CPP`/`WSPUDP.*` socket path instead.
- Drop `CODE/DPMI.CPP` and exclude `WWFLAT32/` from the build entirely (superseded by `WIN32LIB/`, and its DPMI/real-mode code has no meaning on x64 Windows regardless).
- Build and run as **x64 only** throughout — no x86 configuration to maintain.

### Phase 2 — Toolchain migration for what's kept
- Convert the retained `.ASM` files to UASM-assembled objects wired into CMake via a custom command/target. Flag: some 32-bit-flat-model assembly may need adjustment for x64 calling conventions/pointer sizes even without a full C++ rewrite — assess per-file when reached.
- Get `WIN32LIB/` compiling clean under MSVC 2026 (x64) as a static library target in the new CMake build.
- Get `CODE/` compiling against it.

### Phase 3 — Verify
- Confirm the resulting executable links and runs against real game assets (per [../agent_policies/licensing.md](../agent_policies/licensing.md), the user needs to own the game for the assets — this plan only ever touches code).
- Update [../project_state/overview.md](../project_state/overview.md)'s "Build status: does not currently compile" line once genuinely false.
- This is the point where the "get everything working" milestone is complete — Phases 4-5 below are the deferred "ultimate goal" work and don't block calling Phase 0-3 done.

### Phase 4 — Cross-platform renderer + platform-layer rewrite (deferred — the "ultimate goal")
- Replace the DirectDraw-via-shim path with **SDL3** for windowing/input/audio/event handling, replacing `WIN32LIB`'s Windows-specific `KEYBOARD`/`MOUSE`/`WINCOMM`/`PLAYCD`/`TIMER` modules — this is what actually enables a genuine cross-platform (Windows/Linux/macOS) build; Phases 0-3 only reach modern *Windows*.
- Build **two separate, hand-rolled rendering backends**: Vulkan (primary/cross-platform) and Direct3D 12 (Windows-specific) — not SDL3's `SDL_GPU` abstraction.
- Integrate **Dear ImGui** on top of both backends (`imgui_impl_sdl3` + `imgui_impl_vulkan`/`imgui_impl_dx12`) for a debug overlay (successor to `CODE/MONOC.CPP`'s mono-monitor debug screen) and likely a rebuilt map-editor UI — not the in-game HUD, which stays on the original pixel-art `GadgetClass` widget rendering ported to whichever backend is active.
- **Reuse the reference project's `overlay-ports/imgui`** ([../knowledge_base/cmake_vcpkg_reference_patterns.md](../knowledge_base/cmake_vcpkg_reference_patterns.md)) rather than the stock vcpkg registry imgui port — it already pins a newer imgui with `sdl3-binding`, `vulkan-binding`, and `dx12-binding` features all present simultaneously, confirmed by direct inspection.

### Phase 5 — ASM → C++ rewrite of hot paths (deferred backlog)
- Revisit whether the original performance rationale for hand-tuned routines (e.g. `SHAPE/DS_*.ASM`'s 13 blit variants) still holds under modern compilers/hardware; rewrite in C++ where it doesn't, removing the UASM build step entirely once complete.

## Status log
- **2026-09-12**: Plan created; research complete, all five architecture decisions resolved by the project owner (see above).
- **2026-09-12**: **Phase 0 complete.** Added `vcpkg` as a git submodule at `externals/vcpkg` (bootstrapped), root `vcpkg.json` (`directx-headers` dependency), `cmake/Options.cmake` + `cmake/FindDependencies.cmake`, root `CMakeLists.txt`, `CMakePresets.json` (`Main` preset: `Visual Studio 18 2026` generator, x64, vcpkg toolchain file), a root `.gitignore` for `/build/`/`/install/`, and a throwaway `cmake/phase0_smoke_test.cpp` target. **Verified end-to-end, not just authored**: `cmake --preset Main` configured successfully (vcpkg installed `directx-headers` and detected MSVC 19.51/VS2026 toolset), `cmake --build build/Main --config Release` built cleanly, and the resulting `phase0_smoke_test.exe` ran and printed its confirmation line. Compatibility-shim tool for Phase 1 decided: **dxwrapper** (open-source, buildable-from-source fit for the vcpkg-first approach) — note this is a runtime DLL-substitution tool, not a compile-time dependency, so it won't appear in `vcpkg.json`.
- **2026-09-12**: **Sub-milestone 1a complete.** `WIN32LIB` compiles standalone with 0 errors (`win32lib.lib` produced), down from 1650 errors across 25 files at the first real build attempt. Two genuine root-cause bugs found and fixed: `WWSTD.H`'s `#include <windows.h>` guard broke under CMake's default `WIN32` preprocessor define (fixed to check `_WINDOWS_` instead — dropped 1650→180 alone), and `#pragma pack(N)` set before `<windows.h>`'s first inclusion in two audio files (modern Windows SDK headers `static_assert` against exactly this). Several files excluded with documented justification rather than patched (dead/broken stubs, a class only declared behind a DOS-only `#ifdef`, a missing header genuinely absent from the GPL release, a proven-duplicate-definition file). Full record: [../plans_done/win32lib_1a_port_log.md](../plans_done/win32lib_1a_port_log.md).
- **2026-09-12**: **Sub-milestone 1b complete.** `CODE/` compiles and links against `win32lib` with 0 errors (`redalert_code.lib` produced), down from ~20,835 errors against the authoritative 242-file source list transcribed from Westwood's own `MAKEFILE`. Biggest-leverage fixes: a systemic angle-bracket include-path shadowing bug (`CODE/` listed before `WIN32LIB/INCLUDE`, silently swapping in the wrong version of 7 identically-named headers; ~14,000 errors), a circular `FUNCTION.H` self-inclusion via `ABSTRACT.H`/`GSCREEN.H` that left derived classes seeing incomplete bases across the entire `map.h`→`display.h`→...→`target.h` chain, and three missing compile definitions (`WOLAPI_INTEGRATION`/`WINSOCK_IPX`/`ENGLISH=1`) that were silently preprocessing out large chunks of real code (e.g. all of `ToolTipClass`). `PaletteClass` (no surviving implementation anywhere in the GPL release, but needed by ~50 files for palette-fade rendering) was reconstructed from call-site usage after an owner-approved blast-radius estimate. Three files with confirmed-absent proprietary/missing dependencies (Greenleaf Comm Library, `phone.h`, `PassEdit.h`) excluded with justification: `NULLDLG.CPP`, `NULLMGR.CPP`, `WOL_LOGN.CPP`. Full record: [../plans_done/code_1b_port_log.md](../plans_done/code_1b_port_log.md).
- Next action: sub-milestone 1c — get `.ASM` wired in (Phase 2) enough to attempt an actual executable link, then exercise the dxwrapper DirectDraw shim and the miniaudio swap for the first time.
