---
name: modernization-options
description: Researched modern/free replacements for this codebase's legacy toolchain and proprietary dependencies, for a CMake/vcpkg-based rebuild
metadata:
  type: knowledge_base
---

# Modernization Options (researched 2026-09-12)

This documents free/modern replacements researched for a **modernized** rebuild of this codebase — target: CMake + vcpkg (as a git submodule), current MSVC (Visual Studio 2026), contemporary Windows APIs. This is distinct from [dependencies_and_missing_libs.md](dependencies_and_missing_libs.md), which covers what's needed to build the *original* 1996 Watcom/TASM toolchain as-is (still worth keeping documented — restoring the original build is a smaller, different task than modernizing it). The active project plan using these findings is [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md).

## Toolchain replacements (all free, verified 2026-09-12)
- Watcom C/C++ 10.6 → for a modernized build, superseded outright by targeting **MSVC via Visual Studio 2026 (v18.x)** directly — a real, current release (AI-native IDE, Copilot-integrated C++ agents), free via VS Community. [Open Watcom v2](https://github.com/open-watcom/open-watcom-v2) remains a maintained free option (regular 2026 builds, Sybase Open Watcom Public License) if ever restoring the *original* toolchain for closer original-codegen parity matters instead.
- Borland TASM 4.0 → **[UASM](https://www.terraspace.co.uk/uasm.html)** (MASM/TASM-syntax-compatible continuation of JWasm — JWasm itself stopped development in 2014). Only needed for whichever `.ASM` files are kept rather than rewritten in C++ — see open decision #1 below.
- MSVC 6 (`LAUNCHER/`) → folds into the same VS2026/MSVC target; its `.DSP`/`.DSW` project files don't survive the move to CMake regardless.

## DirectX / graphics
- DirectX 5 SDK / DirectDraw → **not needed at all** for a modernized target. D3D11/D3D12 headers and import libraries ship in the Windows SDK (bundled free with Visual Studio); the Agility SDK for newest D3D12 features is a NuGet/vcpkg package (`Microsoft.Direct3D.D3D12` / vcpkg's `directx-headers` + `directx-agility-sdk` ports), no legacy SDK download required.
- Two viable strategies, not mutually exclusive:
  1. **Compatibility-shim** (low effort) — keep `WIN32LIB/MISC/DDRAW.CPP`'s DirectDraw calls unmodified; run under a translation wrapper like [dgVoodoo2](https://dege.freeweb.hu/dgVoodoo2/) or the open-source [dxwrapper](https://github.com/elishacloud/dxwrapper) (DirectDraw→D3D9/11 conversion). Fast path to "runs on modern Windows/GPUs," near-zero source changes.
  2. **Native rewrite** (real project) — replace the DirectDraw surface/blit layer with an actual modern renderer. Given this engine does pure 2D sprite/tile blitting with no 3D at all, **D3D11 is the pragmatic target, not D3D12**: Microsoft's own D3D11→D3D12 porting guidance treats D3D11 as the sane intermediate step even for engines that do go on to D3D12, and D3D12's explicit low-level model (descriptor heaps, command lists, root signatures, fences) buys nothing for a sprite blitter. A cross-platform layer (SDL2, bgfx — both have vcpkg ports) is worth considering if Windows-only isn't a hard requirement.
- `WIN32LIB/MOVIE/`'s DirectShow dependency (needs DirectX Media 5.1 SDK originally) → modern equivalent is **Media Foundation**, DirectShow's Microsoft-designated successor, built into modern Windows, no SDK download needed. Note this module is only used for one FMV playback path (`CODE/MCI.CPP`/`MCIMOVIE.CPP`) — the game's primary cutscene path is the custom VQA codec in `VQ/`/`WINVQ/`, which has nothing to do with DirectShow/Media Foundation at all and needs no replacement here.

## Audio
- HMI "SOS" driver → **[miniaudio](https://miniaud.io/)** (single-header C library, public domain / MIT-No-Attribution, cross-platform, zero external dependencies — likely vendored directly rather than pulled via vcpkg, since it's designed to be dropped in as one header) or **XAudio2** (built into Windows, free, Windows/Xbox-only, "pull"-style mixing API) if staying Windows-only. Only the low-level driver layer (`SOS*.CPP/.H`, present in `WIN32LIB/AUDIO/`, `WWFLAT32/AUDIO/`, `VQ/`/`WINVQ/`) needs replacing — the `.AUD` ADPCM codec logic itself (see [file_formats.md](file_formats.md)) is separate Westwood code and doesn't need to change.

## Networking
- Greenleaf Communications Library → **drop, don't replace**. `WIN32LIB/WINCOMM/` already moved to native Win32 serial APIs, and null-modem/dial-up multiplayer (`CODE/NULLCONN.*`, `CODE/MPMGRD.*`) is an obsolete play-mode nobody would use today. Recommend deleting that path rather than modernizing it.
- `CODE/IPX*.CPP` + top-level `IPX/`'s 16-bit thunk DLLs → **cannot be ported, must be replaced**: 16-bit code cannot execute at all on 64-bit Windows — this is a hard platform wall, not a difficulty tradeoff. Modern equivalent is a plain UDP/TCP socket layer, which `CODE/TCPIP.CPP`/`WSPUDP.*` already partially provide.
- `CODE/DPMI.CPP`, `WWFLAT32/DESCMGMT/`, and all DOS4GW real/protected-mode-bridging ASM → **delete, don't port**. These exist only to bridge real-mode DOS to protected-mode code; there's no real mode on modern Windows at all. Since `WWFLAT32/` is already superseded by `WIN32LIB/` for the Windows build (see [../project_state/modules/wwflat32.md](../project_state/modules/wwflat32.md)), the practical recommendation is to **exclude `WWFLAT32/` from the modernization scope entirely** and build forward from `WIN32LIB/` only.

## Open engineering decisions (need a call from the project owner before Phase 1 of the plan starts)
1. **Keep hand-written ASM (via UASM) or rewrite the hot paths in C++?** Modern compilers auto-vectorize/optimize well enough that the original performance rationale for e.g. `SHAPE/DS_*.ASM`'s 13 hand-tuned blit variants may no longer apply. Rewriting in C++ simplifies the CMake build (no assembler toolchain step) at the cost of a larger per-routine porting effort.
2. **x86 (Win32) or x64 target?** Much of the original code assumes 32-bit flat pointers; x64 is more future-proof but increases porting surface — notably the pointer-relocation logic in `CODE/SAVELOAD.CPP`'s save-game (de)serialization.
3. **D3D11 vs. D3D12 vs. cross-platform (SDL2/bgfx) vs. compatibility-shim** for the rendering layer — see above; this is the single highest-effort decision in the whole project.
4. **`VQ/` vs. `WINVQ/` as the modernization base** for video playback — see [../project_state/modules/vq_winvq.md](../project_state/modules/vq_winvq.md); `WINVQ` carries extra tooling (`VQAVIEW`, `VPLAY32`) not needed at runtime, but its `VQA32` has minor adaptations `VQ/VQA32` lacks.
