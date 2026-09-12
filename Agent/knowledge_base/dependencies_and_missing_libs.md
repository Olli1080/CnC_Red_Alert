---
name: dependencies-and-missing-libs
description: Third-party SDKs and libraries this codebase needs but doesn't fully vendor, and what partial artifacts are present
metadata:
  type: knowledge_base
---

# Dependencies & Missing Libraries

> This file covers what's needed to build the **original** 1996 Watcom/TASM toolchain as-is. For free/modern replacements researched toward a CMake+vcpkg modernized rebuild instead, see [modernization_options.md](modernization_options.md) and the active plan at [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md).

Per the root [README.md](../../README.md), rebuilding requires sourcing (or replacing/removing code that uses) these externally:

## DirectX 5 SDK
Used by `WIN32LIB/MISC/DDRAW.CPP` (DirectDraw setup, `<ddraw.h>`) for video-mode negotiation and surface/palette management — the core 2D rendering backend for the Win32 build. Not vendored at all.

## DirectX Media 5.1 SDK
Used by `WIN32LIB/MOVIE/` (dated May 1998, the newest module in `WIN32LIB/`) for DirectShow-based FMV playback (`IGraphBuilder`, `IMultiMediaStream`, `IDirectDrawMediaStream`, `IDirectDrawStreamSample`). This module notably has **no** `.BOR`/`.WAT` makefile variant, unlike every other `WIN32LIB` module — it was retrofitted only for the toolchain that could reach the DirectX Media SDK. Not vendored.

## Greenleaf Communications Library (GCL)
A commercial DOS/Win serial-comms library. `WIN32LIB/WINCOMM/` explicitly documents itself as **replacing** GCL with native Win32 serial APIs (header comment), so GCL itself is not required by that specific module — but the README still lists it as a dependency, implying some other part of the tree (possibly an older/DOS-era comms path, or `WWFLAT32`) still expects it. Not vendored; not yet located precisely in this pass — candidate for [../Analysis/summary.md](../Analysis/summary.md).

## Human Machine Interface (HMI) "Sound Operating System" (SOS)
A licensed third-party audio driver (© 1993-94 Human Machine Interfaces). **Partially vendored**:
- Headers present: `WIN32LIB/AUDIO/SOS*.H`, `WWFLAT32/AUDIO/SOS*.H`, `VQ/INCLUDE/HMI32/SOS*.H`, `WINVQ/INCLUDE/VQA32/SOS*.H` (vendored flat here instead of a separate `HMI32/` folder).
- Prebuilt `.LIB` binaries present: `WIN32LIB/AUDIO/SOSDW1PS.LIB` (also under `VQ/LIB`'s referenced location), and a fuller set in `WINVQ/LIB/`: `SOSDBLC.LIB`, `SOSDW1CR.LIB`, `SOSDW1CS.LIB`, `SOSDW1PR.LIB`, `SOSDW1PS.LIB`, `SOSMBLC.LIB`.
- ADPCM codec assembly *is* present and appears to be Westwood/HMI co-authored: `WIN32LIB/AUDIO/OLSOSDEC.ASM` (HMI-authored decoder), `SOSCODEC.ASM` (present in `WIN32LIB/AUDIO/`, `WWFLAT32/AUDIO/`, `VQ/VQM32/`, `WINVQ/VQM32/`).
- What's genuinely missing is the **driver's own resident/interrupt-mode binary and full SDK source** — the game links against the prebuilt `.LIB`s rather than building the driver itself.

## `.LIB` naming convention (documented in-repo)
`VQ/LIB/README.TXT` documents Westwood's prebuilt-library naming scheme: `nnnssctm.lib` = 3-char module ID + 32/16-bit size + compiler (Watcom/Borland) + C/C++ type + flat/TNT memory model. Useful for decoding any other `.LIB` filename encountered in the tree.

## To use compiled binaries at all
Per the README, you must own the game — available via [EA App](https://www.ea.com/en-gb/games/command-and-conquer/command-and-conquer-the-ultimate-collection/buy/pc) or [Steam](https://store.steampowered.com/bundle/39394/Command__Conquer_The_Ultimate_Collection/) — for the game assets (`.MIX` files, etc.) that this source code alone does not include.

## Open question for future investigation
Exactly which source file(s) actually require GCL (as opposed to the already-replaced `WINCOMM`) hasn't been pinned down in this pass. See [../Analysis/summary.md](../Analysis/summary.md).
