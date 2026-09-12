---
name: file-formats
description: Westwood's proprietary on-disk/wire formats implemented in this codebase, with the source files that define each
metadata:
  type: knowledge_base
---

# File Formats

Westwood's proprietary formats, as reverse-documented from source during this pass. Useful when writing external tooling (extractors, editors) against original game assets.

## `.MIX` — asset archive container
Implemented by [CODE/MIXFILE.CPP/.H](../project_state/modules/code/file_io_compression_saveload.md) (game-side reader) and [TOOLS/MIX/MIXFILE.CPP](../project_state/modules/ipx_launch_launcher_tools.md) (the packer tool, `DataClass`). **Confirmed on-disk layout** (from the packer's `Process_Output`):
```
short Count       -- number of entries
long  TotalSize
Count x { long CRC; long Offset; long Size; }   -- fixed 12-byte records, sorted by CRC (binary-searchable)
<concatenated raw file bytes, in original input-list order>
```
CRC is computed from the (uppercased) output filename. This is the original C&C/RA `.MIX` layout — no later Tiberian-Sun-era encryption/flag-byte header. Also referenced by `VQ/INCLUDE/VQM32/MIXFILE.H` for the VQA-side tools.

## `.AUD` — compressed audio
Header/codec logic in `TOOLS/AUDIOMAK/AUDIOMAK.CPP` (the encoder) and [WIN32LIB/AUDIO](../project_state/modules/win32lib.md) / [WWFLAT32/AUDIO](../project_state/modules/wwflat32.md) / [CODE/ADPCM.CPP](../project_state/modules/code/platform_audio_misc.md) (decoders). Output header fields: `Rate`, `Size`, `Stereo:1`, `Bits:1`, `Compression` enum — `SCOMP_NONE=0`, `SCOMP_WESTWOOD` (custom sliding-window delta compression, implemented in `TOOLS/AUDIOMAK/SCODE.ASM` and decoded via `OLSOSDEC.ASM`/`SOSCODEC.ASM`), `SCOMP_SONARC=33` (third-party SONARC pre-compression passthrough).

## `.WSA` — delta-compressed animation
XOR-delta frame encoding. Playback in [WIN32LIB/WSA](../project_state/modules/win32lib.md)/[WWFLAT32/WSA](../project_state/modules/wwflat32.md) (`WSA.H`/`.CPP`, `XORDELTA.ASM`'s `Apply_XOR_Delta`/`Copy_Delta_Buffer`). Used for in-engine animations like building construction, not full cutscenes (see `.VQA` below).

## `.VQA` — Vector Quantized Animation (video codec)
Full detail in [project_state/modules/vq_winvq.md](../project_state/modules/vq_winvq.md). Summary: an IFF (`FORM`/chunk) container with form ID `WVQA`, holding per-frame vector-pointer indices into a periodically-refreshed shared codebook of pixel-block patterns (2x2/2x3/4x2/4x4 configurable). Chunk IDs: `VQHD` (header), `FINF` (frame offsets), `VQFR`/`VQFK` (frame data), `CBF0`/`CBFZ`/`CBP0`/`CBPZ` (codebooks), `VPT0`..`VPRZ` (vector pointers, with a "Run-Skip-Dump" run-length scheme), `CPL0`/`CPLZ` (palette), `SND0`..`SNDZ`/`SNA0`..`SNAZ` (audio), `CAP0`/`EVA0` (subtitle/EVA caption text). A distinct, simpler still-image `VQ` format also exists (`VQHR`/`VQCB`/`VQCT`/`VQVP` chunks, defined in `VQ/INCLUDE/VQ.H`), with a second, non-identical field layout for the same concept in `WINVQ/INCLUDE/VQFILE.H` (format drift between the two trees — flagged, not yet resolved).

## Compression/crypto filters — the "Straw & Pipe" framework
Core abstraction in [CODE/PIPE.CPP/.H, CODE/STRAW.CPP/.H](../project_state/modules/code/file_io_compression_saveload.md): a `Pipe` is a stackable write-side filter chain, a `Straw` a stackable read-side filter chain, so any combination of the below can wrap file/buffer I/O transparently:
- **LCW** — Westwood's own RLE+dictionary compression (by Louis Castle), the most pervasive codec in the whole codebase (used for images, `.MIX`-adjacent formats, and VQA container compression). `LCWCOMP.ASM`/`LCWUNCMP.ASM` appear near-identically in `CODE/`, `WIN32LIB/IFF/`, `WWFLAT32/IFF/`, `VQ/VQM32/`, `WINVQ/VQM32/`.
- **LZO** (`LZO1X_C/D`), **LZW**, **Blowfish**, **SHA**, **CRC**, **PK**, **Base64** — each has a matching `*PIPE`/`*STRAW` pair in `CODE/` (e.g. `LZOPIPE`/`LZOSTRAW`, `BLOWPIPE`/`BLWSTRAW`, `CRCPIPE`/`CRCSTRAW`). CRC filters are used for multiplayer desync detection (checksumming game state across networked machines), not just file integrity.
- **Huffman** (order-0) — `HUFFCMP.CPP`/`HUFFDCMP.ASM` in `VQ/VQM32`/`WINVQ/VQM32`, used for VQA container-level compression alongside LCW.
- **Audio "Zap"** — `AUDZAP.CPP`/`AUDUNZAP.ASM` in `VQ/VQM32`/`WINVQ/VQM32`, a lossy delta scheme for VQA's embedded audio track (distinct from the main `.AUD` format's compression).

## `.INI`-based data (rules, scenarios, mix control files)
- `CODE/INI.CPP/.H` — generic `INIClass` text-config parser.
- `CODE/CCINI.CPP/.H` — Red-Alert-specific `CCINIClass` extension.
- `CODE/INIBIN.CPP`/`INICODE.CPP` — binary/compiled variant, used for performance-sensitive rules/scenario loading.
- `CODE/RULES.CPP/.H` — parses `RULES.INI`-style gameplay balance data (fixed-point armor/speed/cost/build-speed biases per difficulty) via `RulesClass`/`DifficultyClass`.
- `TOOLS/MIX/MIXFILE.CPP`'s **control file** format is a separate, much simpler plain-text format: one filename per line/whitespace, consumed via `fscanf("%s")` — not an `.INI` file, don't confuse the two.

## Image formats
`.PCX`, `.LBM`(IFF-ILBM) loading/writing implemented in [WIN32LIB/IFF](../project_state/modules/win32lib.md) and [WWFLAT32/IFF](../project_state/modules/wwflat32.md) (`LOADPCX.CPP`, `WRITEPCX.CPP`, `WRITELBM.CPP`), layered on the LCW codec above. `.TGA` (Targa) loading exists separately in `VQ/VQM32/TARGA.CPP` for VQA-tooling purposes.
