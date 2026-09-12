---
name: code-file-io-compression-saveload
description: CODE/ Straw and Pipe filter framework, file abstraction hierarchy, .MIX reader, INI parsing, save/load pipeline
metadata:
  type: project_state
---

# CODE/ — File I/O, Compression & Save/Load

Parent index: [summary.md](summary.md). File formats produced/consumed by this code are cataloged in [../../../knowledge_base/file_formats.md](../../../knowledge_base/file_formats.md) — this file focuses on the source structure.

## The "Straw & Pipe" I/O framework
The core abstraction underlying nearly all file/network I/O in the engine:
- `PIPE.CPP`/`.H`, `STRAW.CPP`/`.H` — a `Pipe` is a write-side filter chain, a `Straw` a read-side filter chain; both are stackable so encryption/compression/checksums compose transparently over any file/buffer I/O.
- `XPIPE.CPP`/`.H`, `XSTRAW.CPP`/`.H` — base pass-through pipe/straw plumbing.

### Filter implementations (each a `*PIPE`/`*STRAW` pair over a codec)
- `LCW.CPP`/`.H`, `LCWPIPE.*`, `LCWSTRAW.*`, `LCWUNCMP.CPP`, `LCWCOMP.ASM` — Westwood's proprietary LCW run-length compression (ASM-accelerated encode), the most pervasive codec in the codebase.
- `LZO1X_C`/`LZO1X_D`, `LZOCONF`, `LZOPIPE`, `LZOSTRAW` — LZO compression library integration.
- `LZW.CPP`/`.H`, `LZWPIPE.*`, `LZWSTRAW.*`, `LZWOTRAW.CPP` — LZW compression variant.
- `BLOWFISH.CPP`/`.H`, `BLOWPIPE.*`, `BLWSTRAW.*` — Blowfish encryption (online/CD-key or save security).
- `SHA.CPP`/`.H`, `SHASTRAW.*`, `SHAPIPE.*` — SHA hashing filter.
- `CRC.CPP`/`.H`, `CRCPIPE.*`, `CRCSTRAW.*` — CRC checksum filter, used for multiplayer desync detection (checksumming game state across networked machines).
- `PK.CPP`/`.H`, `PKPIPE.*`, `PKSTRAW.*` — Westwood "PK" compression/packing filter, underlies `MIXFILE.CPP` below.
- `B64PIPE.*`, `B64STRAW.*`, `BASE64.CPP`/`.H` — Base64 encode/decode.
- `BIGCHECK.CPP`/`.H` — big-value checksum helper (cheat/mod detection).
- `CSTRAW.CPP`/`.H` — additional filter glue.

## File abstraction hierarchy
- `FILE.CPP`, `CCFILE.CPP`/`.H`, `RAWFILE.CPP`/`.H`, `CDFILE.CPP`/`.H`, `RAMFILE.CPP`/`.H`, `BFIOFILE.CPP`/`.H` — generic `FileClass`, CD-ROM-aware file access, raw disk file, in-RAM virtual file, buffered I/O file.
- `MIXFILE.CPP`/`.H` — `MixFileClass`: reader for Westwood's `.MIX` archive container format, built on the `pk.h` framework. See [../../../knowledge_base/file_formats.md](../../../knowledge_base/file_formats.md) for the on-disk layout (verified from the packer tool in [../ipx_launch_launcher_tools.md](../ipx_launch_launcher_tools.md)).
- `CCMPATH.CPP` — path-resolution helper for locating CD/game asset files.

## Config / data parsing
- `INI.CPP`/`.H` — generic `INIClass` text-config parser.
- `CCINI.CPP`/`.H` — Red-Alert-specific `CCINIClass` extension.
- `INIBIN.CPP`, `INICODE.CPP` — binary/compiled variant used for performance-sensitive rules/scenario data loading.
- Consumed by `RULES.CPP` (see [ai_scripting_scenario.md](ai_scripting_scenario.md)) and `SCENARIO.CPP`.

## Save / load
- `SAVELOAD.CPP` — `Save_Game`/`Load_Game`/`Code_All_Pointers`/`Decode_All_Pointers`: the full save-game (de)serialization pipeline, including relocating in-memory pointers to/from stable IDs and multiplayer-specific value save/restore. On-disk format not yet reverse-documented — see [../../../Analysis/summary.md](../../../Analysis/summary.md).
- `SAVEDLG.H` — save dialog UI declarations (paired with `LOADDLG.CPP`, which handles both load and save dialog UI logic — see [map_display_ui.md](map_display_ui.md)).
