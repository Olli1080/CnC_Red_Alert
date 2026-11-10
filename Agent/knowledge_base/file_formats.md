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
XOR-delta frame encoding. Playback in [WIN32LIB/WSA](../project_state/modules/win32lib.md)/[WWFLAT32/WSA](../project_state/modules/wwflat32.md) (`WSA.H`/`.CPP`, `XORDELTA.ASM`'s `Apply_XOR_Delta`/`Copy_Delta_Buffer`). Used for in-engine animations like building construction, the score screen and the **campaign map** (`MSAA..MSAD.WSA`, `MSSA/MSSB.WSA`, 70 frames, 320x200), not full cutscenes (see `.VQA` below). Port-relevant layout facts (verified against the owner's files, pitfalls #25):
- On-disk header = **22 bytes** (7 `unsigned short` + `frame0_offset`/`frame0_end` as `unsigned long`; no padding), followed by `(total_frames+2)` 32-bit frame offsets, an optional 768-byte palette (`flags & 1`) and the LCW-compressed XOR-delta frames. `WSA_FILE_HEADER_SIZE` (offsets relative to the file) is 14 bytes: the header without the two `frame0_*` longs.
- `largest_frame_size` in the file = real largest delta + `sizeof(SysAnimHeaderType)` of the 32-bit Watcom build (the authoring tool added it); `Open_Animation` subtracts it again. With `/zp1` the 32-bit in-memory header is **43 bytes** (`WSA_ORIGINAL_HEADER_SIZE`), and the delta buffer is exactly that sized.
- Frames are **decompressed in place**: the compressed frame is copied to the *end* of the delta buffer and LCW expands towards it; the buffer is sized to the byte, so the decoder must honour the `length` argument and never write past a run (`LCW_Uncompress_Bounded`, `CODE/LCWUNCMP.CPP`). The XOR delta is then applied to the target page with the destination **row stride** (not a skip value).

## `.VQA` — Vector Quantized Animation (video codec)
**Which movies exist in the owner's data (Steam Remastered Collection):** only `SIZZLE.VQA`/`SIZZLE2.VQA` (in `COUNTERSTRIKE\`) are classic VQA files (`FORM....WVQA`, `VQHD` header; SIZZLE: 320x156, 451 frames, 15 fps). All other Red Alert movies are Remastered HD Bink 2 (`.BK2`, signature `KB2j`) inside the 4 GB `Data\MOVIES_RA.MEG` (MEG v2: magic `FFFFFFFF 3F7D70A4`). In this port `CODE/VQASTUB.CPP` stubs the player; see [../plans_wip/movies.md](../plans_wip/movies.md).
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
