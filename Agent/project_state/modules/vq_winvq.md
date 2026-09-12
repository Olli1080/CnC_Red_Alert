---
name: vq-winvq
description: The VQA video codec system — engine-integrated library (VQ/) vs SDK/tools packaging (WINVQ/)
metadata:
  type: project_state
---

# VQ/ + WINVQ/ — VQA Video Codec System

Parent index: [summary.md](summary.md). Format details cross-referenced in [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md).

## Overview: what VQA is
"Vector Quantized Animation" — Westwood's proprietary full-motion-video codec (1994-95-era, not a third-party wrapper) used for cutscenes/briefings. Classic vector quantization over fixed-size pixel blocks (2x2/2x3/4x2/4x4, build-time selectable): each frame is a grid of indices into a shared codebook of pixel-block patterns, refreshed every "Groupsize" frames rather than every frame. Vector-pointer streams support a "Run-Skip-Dump" run-length scheme. Container compression: LCW + order-0 Huffman; audio: a lossy "Zap" ADPCM-like scheme and/or HMI's SOS ADPCM codec. On-disk container is IFF form `WVQA` — full chunk-ID list in [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md). **Decode-only**: no encoder exists in this repo; codebooks/streams were produced by external Westwood tools not included here. Playback splits into three cooperating stages: **Loader** (streams/decompresses chunks ahead into circular buffers), **Drawer** (runs `UnVQ_2x2`/`UnVQ_4x2`/etc. to blit codebook blocks via vector pointers, manages palette/page-flip), **Flipper** (presents completed frames) — synced to a selectable timer. A second, simpler **still-image "VQ" format** (distinct from the `WVQA` movie format) also exists, with **format drift**: `VQ/INCLUDE/VQ.H`'s `_VQHeader` and `WINVQ/INCLUDE/VQFILE.H`'s `VQHeader` define non-identical field layouts for the apparently-same concept — unresolved, see [../../Analysis/summary.md](../../Analysis/summary.md).

## VQ vs. WINVQ: not a DOS/Windows split
Both target Watcom/DPMI or Borland+PharLap-TNT DOS-extended protected mode — **not** analogous to WWFLAT32/WIN32LIB. The real distinction is **library-only vs. library-plus-tools**:
- **`VQ/`** — the trimmed copy integrated directly into the Red Alert game engine: just `VQM32` (codec/compression/video primitives) + `VQA32` (movie playback engine) + public `INCLUDE` headers.
- **`WINVQ/`** — the same libraries (byte-identical `VQM32`; `VQA32` with real diffs in `AUDIO.CPP`/`DRAWER.CPP` adapted for a tool/TNT host environment) repackaged as an SDK, **plus two consumer apps not in `VQ/`**: `VQAVIEW` (Win32 GUI viewer) and `VPLAY32` (DOS console player), plus prebuilt third-party `.LIB` binaries `VQ/LIB` lacks. `WINVQ/VQAVIEW`'s own makefile header reads *"Project Name: Blade Runner"* — this tool was built for Westwood's *Blade Runner* (1997) and vendored into the RA tree (see [../../knowledge_base/codebase_history_and_cruft.md](../../knowledge_base/codebase_history_and_cruft.md)).

## VQ/INCLUDE
Public headers, mirrored under subfolders by owning library: top-level `VQ.H` (still-image format), `VOCFILE.H`/`WAVEFILE.H` (Creative VOC/RIFF WAVE structs for optional non-VQA audio), `WWTYPES.H`. `VQA32/` — `VQAFILE.H` (the `WVQA` chunk/container format), `VQAPLAY.H` (public `VQA_Open`/`VQA_Play`/`VQA_Close` API, `VQAConfig`/`VQAInfo`/`VQAStatistics`; `VQAPLAY.BAK` = stale pre-1995 draft). `VQM32/` — `IFF.H`, `COMPRESS.H`, `CRC.H`, `HUFFMAN.H`, `MIXFILE.H`, `PALETTE.H`, `TARGA.H`, `VIDEO.H`/`VESAVID.H`, `FONT.H`/`TEXT.H`, `MEM.H`, `PROFILE.H`, `REALMODE.H` (DPMI thunking), `SOSCOMP.H`, `CAPTOKEN.H`. `HMI32/` — vendored `SOS*.H`. `WWLIB32/` — broader Westwood game-library headers pulled in as a dependency (`GBUFFER.H`, `KEYBOARD.H`, `SHAPE.H`, `TILE.H`, `AUDIO.H`, `WSA.H`).

## VQ/LIB
Only `README.TXT`, documenting the prebuilt-library naming convention (`nnnssctm.lib`) — see [../../knowledge_base/dependencies_and_missing_libs.md](../../knowledge_base/dependencies_and_missing_libs.md). No actual `.LIB` binaries checked in (placeholder for build output).

## VQ/VQA32 — the movie playback engine
`LOADER.CPP` (`VQA_Open`/`Close`, stream-ahead loading and IFF chunk pre-processing — largest/central file), `DRAWER.CPP` (`VQA_Configure_Drawer`, frame drawing/page-flip, selects the `UnVQ_*` routine per block size/video mode), `AUDIO.CPP` (timer + SOS digital audio), `TASK.CPP` (`VQA_Alloc`/`Free`, top-level scheduler tying Loader/Drawer together), `CONFIG.CPP` (INI/default config), `CAPTION.CPP`+`CAPTOKEN.CPP`/`.H` (subtitle/EVA caption tokenizing), `DSTREAM.CPP` (default DOS file-I/O handler), `MONODISP.CPP` (mono-monitor debug), `VERTAG.CPP` (embeds version string). Per-pixel decode in ASM: `UNVQBUFF.ASM` (`UnVQ_2x2/2x3/4x2/4x4`, the core codec routine), `UNVQVESA.ASM` (VESA 32K-color direct decode), `UNVQXMDE.ASM` (Xmode/VRAM-resident decode). Headers: `VQAFILE.H`/`VQAPLAY.H`/`VQAPLAYP.H` (private per-instance state: `VQAHandleP`, `VQALoader`, `VQADrawer`, `VQAAudio`). Triple build path: `MAKEFILE`, `MAKEFILE.WAT`, `MAKEFILE.BOR`+`BCC32.CFG`/`TASM32.CFG`/`TLIB.CFG`/`TLINK32.CFG`. Dead files: `AUDIO.BAK1`/`BAK2`, `DRAWER.BAK`, `LOADER.BAK`, `MONODISP.BAK`, `MAKEFILE.BAK`, `UNVQXMDE.ASM.BAK`, `VQAPLAY.BAK`.

## VQ/VQM32 — codec/compression/video primitives
General-purpose, reused across multiple Westwood titles. Compression: `LCWCOMP.ASM`/`LCWUNCMP.ASM` (Louis Castle), `HUFFCMP.CPP`/`HUFFDCMP.ASM`, `AUDZAP.CPP`/`AUDUNZAP.ASM` (Joe Bostic's lossy audio "Zap"), `SOSCODEC.ASM`, `CRC.ASM`/`.H`. Formats: `IFF.CPP`/`.H` (generic IFF reader/writer shared by VQ and VQA containers), `MIXFILE.CPP`/`.H`, `TARGA.CPP`/`.H`. Video: `VIDEO.CPP`/`.H`, `VESAVID.CPP`/`.H`, `VESABLIT.CPP`/`.H`, `XMODEPG.CPP`/`XMODE.ASM`, `MCGABUF.ASM`, `VESABUF.ASM`, `PALETTE.ASM`/`.H`, `SORTPAL.CPP` (luminance/HSV palette sort), `FILLRECT.ASM`, `DRAWCHAR.ASM`/`TEXTPRNT.ASM`/`FONT.CPP`/`.H` (bitmap font), `MONO.ASM`/`.H`. Support: `MEM.CPP`/`.H` (DPMI page lock), `PROFILE.CPP`/`.H` (INI parsing), `PORTIO.ASM`/`.H`, `TESTVB.CPP` (vertical-blank polarity test).

## WINVQ/INCLUDE
Same role as `VQ/INCLUDE`, near-identical, but lacks `HMI32`/`WWLIB32` subfolders (headers vendored flat in `INCLUDE/VQA32/` instead) and uniquely adds top-level `VQFILE.H` (the still-image format drift noted above). `INCLUDE/VQA32/VQAPLAY.H.BAK` is dead.

## WINVQ/LIB
Same `README.TXT` plus **actual prebuilt `.LIB` binaries**: `SOSDBLC.LIB`, `SOSDW1CR.LIB`, `SOSDW1CS.LIB`, `SOSDW1PR.LIB`, `SOSDW1PS.LIB`, `SOSMBLC.LIB` — HMI SOS sound-driver libraries, third-party prebuilt dependencies, not VQA-codec output.

## WINVQ/VQA32
File-for-file matching `VQ/VQA32`'s manifest, built for Borland+PharLap-TNT here. Real diffs from `VQ/VQA32` in `AUDIO.CPP` (~1060/1300 lines differ) and `DRAWER.CPP` (~250 lines) — adapted timer/audio/drawing for the tool/TNT host. `OLD/` = full earlier snapshot (pre-rewrite). Dead: `MAKEFILE.BAK`, `VQAPLAY.BAK`.

## WINVQ/VQM32
File-for-file identical to `VQ/VQM32` (sampled `IFF.CPP` diffed as exactly identical). Only difference: which stray backup is present (`DRAWCHAR.BAK` here vs. `MAKEFILE.BAK` in `VQ/VQM32`).

## WINVQ/VPLAY32 — standalone DOS command-line player
Not a library — `PLYVQA32.CPP` (~1000 lines) has a real `main()`, explicitly described in its own header as "a stand-alone VQA Player, as well as an example of how to incorporate VQAPlay into a program" — Westwood's reference/integration-test client for the VQA32 API (`Usage`, `Options` parsing, `Find_File_Name`, `Print_Play_Stats`, keyboard polling). `CAPTION.FNT`/`EVA.FNT` (subtitle font resources), `VPLAYTNT.DEF` (links against the PharLap TNT DOS-extender runtime stub). No prebuilt `.EXE` checked in.

## WINVQ/VQAVIEW — standalone Win32 GUI viewer
The only true GUI app in either tree — `MAIN.CPP` has a genuine `int WINAPI WinMain(...)`. `MAINWIND.CPP`/`.H` (window/message loop), `WM.CPP`/`.H` (message helpers), `MOVIES.CPP`/`.H` and `VQ.CPP`/`.H` (`VQAClass` wrapper: `Initialize_VQA_System`, `Open_And_Load_Buffers`, `Seek_To_Frame`, `Play_VQA`, `VQ_Drawer_Callback` — glues VQA32 into a Windows GDI/`GBUFFER` surface), `PAL.CPP`/`.H` (syncs VQA palette to the Windows system palette), `DIALOGS.RC`/`.RH`, `MENUS.RC`/`.RH` (Win32 resource scripts), `DEBUG.CPP`/`.RC` and `MONOCHRM.CPP`/`.H` (debug console, the latter explicitly borrowed from Microsoft's `DBWIN.EXE`), `WESTWOOD.H`/`WWLIB.H` (pull in the separate `WIN32LIB`, see [win32lib.md](win32lib.md), via required `%WIN32LIB%` env var). Depends at link time on VQA32/VQM32 object code via `$(BLADE)\VQA\LIB`. Prebuilt `VQAVIEW.EXE` (417,458 bytes) and `VQAVIEW.IDE` checked in; `VQAVIEW.IDE.BAK` and stub `TEST.MAK` are dead.
