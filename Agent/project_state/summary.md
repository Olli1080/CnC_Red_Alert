# Project State — Index

The structural map of the codebase: what every part of it does, down to key-file-level descriptions.

- **[overview.md](overview.md)** — What this repository is as a whole, its top-level layout, and how the ~2,900 files break down by module and by language/extension.
- **[modules/summary.md](modules/summary.md)** — Per-module documentation: [CODE/](modules/code/summary.md) (main game logic, split into 8 sub-files), [WIN32LIB/](modules/win32lib.md), [WWFLAT32/](modules/wwflat32.md), [VQ/ + WINVQ/](modules/vq_winvq.md), [IPX/ + LAUNCH/ + LAUNCHER/ + TOOLS/](modules/ipx_launch_launcher_tools.md).

## Coverage notes
This documentation was built by five parallel investigation passes (2026-09-12), each reading a representative sample of files per module (headers, class declarations, top-of-file comments) rather than every single file. Coverage strategy:
- **Architecturally central files** (base classes, entry points, core algorithms) are individually named and described.
- **Repetitive/formulaic file families** (e.g. ~40 trigger/team-type `.CPP`/`.H` pairs, 13 shape-drawing `DS_*.ASM` variants, per-RTTI `*DATA.CPP` files) are described as a group with the pattern explained, not enumerated file-by-file.
- **Prebuilt binaries and generated files** (`.EXE`, `.LIB`, `.IDE`, MIDL-generated COM headers) are noted as such rather than analyzed as source.
Gaps deliberately left for later are tracked in [../Analysis/summary.md](../Analysis/summary.md).
