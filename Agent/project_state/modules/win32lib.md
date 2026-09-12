---
name: win32lib
description: Westwood's Win32-native low-level support library — one section per subdirectory
metadata:
  type: project_state
---

# WIN32LIB/ — Win32 Support Library

Parent index: [summary.md](summary.md). Historical relationship to [wwflat32.md](wwflat32.md): [../../knowledge_base/codebase_history_and_cruft.md](../../knowledge_base/codebase_history_and_cruft.md).

## Overview
Westwood's proprietary low-level Win32 support library ("WWLIB32") beneath [CODE/](code/summary.md): file/mix-file I/O, palette/DirectDraw graphics buffers and blitting, sprite ("shape") and tile rendering, font/text printing, keyboard/mouse input, digitized/MIDI audio (via HMI's licensed "SOS" driver), WSA animation and IFF/PCX/LBM image loading, CD audio, serial/modem comms, a sampling profiler, a monochrome debug-monitor channel, and general utilities. Descended from the earlier DOS-era WWLIB ([wwflat32.md](wwflat32.md)), ported to 32-bit flat-model Win32 for Red Alert (1996), plus one later addition (`MOVIE/`, DirectShow-based, 1998). ~23 sub-libraries, each historically built as its own static `.LIB` (see [../../knowledge_base/build_toolchains.md](../../knowledge_base/build_toolchains.md)) then linked into WWLIB32. Performance-critical code is TASM `IDEAL`/`P386`/`MODEL USE32 FLAT` assembly (166 `.ASM` files total); the class/API layer is Watcom C++. `INCLUDE/` is the flattened public-header directory; `SRCDEBUG/` is a flattened debug-source snapshot (see below).

## AUDIO
Digitized sound-effect/music playback on HMI's "SOS" driver plus Westwood's `.AUD` streaming/compression (see [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md)). Key: `AUDIO.H` (public API, AUD header struct, device-type enums), `SOSDEFS.H`/`SOSCOMP.H`/etc. (SOS driver interface), `SOUNDINT.CPP` (interrupt-context mixing, memory-locked), `SOUNDIO.CPP` (bulk streaming/file I/O, ~90KB), `SOUNDLCK.CPP` (locks driver-resident memory). ASM: `AUDUNCMP.ASM`, `OLSOSDEC.ASM` (HMI-authored), `SOSCODEC.ASM` (ADPCM codec). Third-party HMI-licensed code embedded directly (see [../../knowledge_base/dependencies_and_missing_libs.md](../../knowledge_base/dependencies_and_missing_libs.md)). `OLD/` = superseded duplicate snapshot.

## DIPTHONG
Small digram/diatomic text-compression codec for English strings (in-game string tables). `DIPTHONG.H`/`.CPP` (API: `Dip_Text`, `UnDip_Text`, `Extract_String`, `Fixup_Text`, dating to 1992), `_DIPTABL.CPP` (dictionary table). No assembly.

## DRAWBUFF
Core 2D graphics primitive layer — pixel/line/rect drawing, blitting, scaling, clipping, palette remapping, icon-stamp caching — on the `Buffer`/`GraphicBuffer`/`GraphicViewPort` class hierarchy used everywhere in the engine. `DRAWBUFF.H` (function-pointer-driven primitive API, global `LogicPage`), `GBUFFER.H`/`.CPP` (class definitions; documents the swappable-backend design so MCGA vs SVGA/VESA can link in without code changes), `BUFFER.H`/`.CPP` (base `BufferClass`), `ICONCACH.H`/`.CPP` (icon/stamp cache). Extensive ASM: `BITBLIT.ASM`, `DRAWLINE.ASM`, `FILLRECT.ASM`/`FILLQUAD.ASM`, `PUTPIX.ASM`/`GETPIX.ASM`, `REMAP.ASM` (palette remap blit), `SCALE.ASM`, `SHADOW.ASM`, `STAMP.ASM`/`STMPCACH.ASM`, `TXTPRNT.ASM`. `TEST/` = standalone test harness.

## EXAMPLE
Minimal "hello world" template (`libtest`) demonstrating linking against WWLIB32. Not shipped. `MAIN.CPP` (trivial `Main_Program` stub), `STARTUP.CPP`, `WWLIB32.H` — the umbrella public header aggregating nearly every module's own header, effectively the master public API entry point.

## FONT
Bitmap font loading and proportional text printing. `FONT.H`/`.CPP` (pixel-width calc, print position), `LOADFONT.CPP`, `SET_FONT.CPP`. ASM: `SETFPAL.ASM` (font palette range), `TEXTPRNT.ASM` (glyph-blit renderer).

## IFF
Image/picture file I/O (IFF/PCX/LBM), on top of LCW compression. `IFF.H` (`MAKE_ID` FOURCC macro, compression-type enum), `IFF.CPP`, `LOAD.CPP`/`LOADPCX.CPP`/`LOADPICT.CPP`, `WRITELBM.CPP`/`WRITEPCX.CPP`, `FILEPCX.H`. ASM: `LCWCOMP.ASM`/`LCWUNCMP.ASM` (Westwood's signature compression, by Louis Castle — see [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md)), `PACK2PLN.ASM` (Amiga-style bitplane packing).

## INCLUDE
Flattened shared public-header directory (60 files) — one copy of essentially every module's `.H`/`.INC` so any consumer can `#include` without per-module `-I` paths. Contains the umbrella `WWLIB32.H` and foundational `WWSTD.H` (base types/macros). `DESCMGMT.H` has no matching `.CPP` anywhere except `SRCDEBUG/` — its owning module was folded away (compare [wwflat32.md](wwflat32.md)'s `DESCMGMT`).

## KEYBOARD
`WWKeyboardClass` (virtual-key/shift/ctrl/alt/release bit-flags) and `WWMouseClass` (cursor draw/erase/hide/show via a `GraphicViewPortClass`). `KEYBOARD.H`/`.CPP`, `MOUSE.H`/`.CPP`. ASM: `WWMOUSE.ASM` (cursor sprite draw/restore). **Heavy legacy cruft**: `OLD/` holds a full superseded DOS-era implementation (real/protected-mode keyboard ISR variants, page-fault handler) plus 5 `.BAK` files; `OLDTEST/`/`TEST/` are dead duplicate test-harness folders.

## MEM
Custom memory-pool allocator with reference-counted/aging cache eviction, plus `new`/`delete` overrides. `WWMEM.H`/`MEMFLAG.H` (API), `MEM.CPP` (pool impl: oldest-block eviction, GC, doubly-linked cache), `ALLOC.CPP`, `NEWDEL.CPP`. ASM: `MEM_COPY.ASM` (fast memcpy), `VMPAGEIN.ASM` (forces VM pages resident, relevant to AUDIO's locked-memory needs). Two parallel duplicate copies exist: `MSVC/` and `OLDMEM/` — history cruft, not distinct functionality.

## MISC
Grab-bag: DirectDraw setup/video-mode management, CRC, random numbers, screen-shake, palette fade tables, sprite-facing lookup tables, OS detection, argv parsing, version stamping. `MISC.H` (pulls in `<windows.h>`/`<ddraw.h>`), `DDRAW.CPP` (DirectDraw object/surface setup), `IRANDOM.CPP`/`RANDOM.ASM`, `FINDARGV.CPP`, `VERSION.CPP`. ASM: `CLIPRECT.ASM`, `CRC.ASM`, `DETPROC.ASM`, `FACING8/16/FF.ASM` (unit-direction rotate lookup tables), `FADING.ASM`, `OPSYS.ASM`, `REVERSE.ASM` (endian reversal, used by IFF), `SHAKESCR.ASM`.

## MONO
Debug-only monochrome second-monitor output (Westwood-signature dev tool). `MONO.H` (`MonoClass`, box-drawing charset), `MONO.CPP`.

## MOVIE
FMV/cutscene playback via DirectShow into a DirectDraw surface — **newest module** (May 1998), likely for a later Win95/98 re-release needing modern video codecs. `MOVIE.H` — `Movie` class wrapping `IGraphBuilder`/`IMultiMediaStream`/`IDirectDrawMediaStream`/`IDirectDrawStreamSample`. Only plain `MAKEFILE` (no `.BOR`/`.WAT`) — not retrofitted to the dual-toolchain build (needs DirectXMedia SDK, see [../../knowledge_base/dependencies_and_missing_libs.md](../../knowledge_base/dependencies_and_missing_libs.md)).

## PALETTE
256-color VGA/DirectDraw palette management. `PALETTE.H` (`PALETTE_SIZE`=256 etc.), `PALETTE.CPP`, `LOADPAL.CPP`, `MORPHPAL.CPP` (interpolated fade transitions). ASM: `PAL.ASM`.

## PLAYCD
Audio-CD ("Red Book") playback control for in-game music. `PLAYCD.H` (largely `NOT_FOR_WIN95`-guarded legacy DOS defs), `GETCD.CPP`, `REDBOOK.CPP`. ASM: `PLAYCD.ASM`.

## PROFILE
Sampling code profiler — a second thread periodically samples the instruction pointer via `GetThreadContext` (can't attribute time inside DirectDraw/DirectSound/VMM, per its own header). `PROFILE.H`/`.CPP`, `WPROFILE.CPP`. ASM: `APROFILE.ASM`. Only module (besides WINCOMM/MOVIE) lacking a `.BOR`/`.WAT` pair. `UTIL/` = lone leftover alternate copy.

## RAWFILE
Low-level file I/O plus the mixfile-transparent "CC file" layer — the file-access backbone nearly every other module loads through. `RAWFILE.H`/`.CPP` (`RawFileClass`), `CCFILE.CPP` (`CCFileClass`, mixfile-transparent open/read/seek). No assembly.

## SHAPE
Compressed sprite ("shape") loading and drawing — core unit/building/cursor sprite format. The single most assembly-heavy module: `SHAPE.H` (flag enums), `GETSHAPE.CPP`, `SETSHAPE.ASM`, `PRIOINIT.CPP` (draw-priority/z-order init), `DRAWSHP.ASM` (dispatcher) + 13 `DS_*.ASM` variants (`DS_DN`, `DS_DR`, `DS_DS`, `DS_DSR`, `DS_LRS`, `DS_LS`, `DS_LSRS`, `DS_LSS`, `DS_RRS`, `DS_RS`, `DS_RSRS`, `DS_RSS`, `DS_TABLE.ASM`) — one hand-tuned inner blit loop per flip/remap/scale combination, to avoid branchy generic code.

## SRCDEBUG
**Not a functional module** — a flattened, no-makefile snapshot of ~169 source files gathered from most other WIN32LIB modules, plus the only surviving copies of three vanished modules: a **FILE cache** module (`FILECACH.CPP` etc.), a **VESA** SVGA-mode module (`VESA.ASM`, `VESAINFO.CPP`, etc.), and a **Video Buffer** module (`VBUFFER.CPP`, `VBITBLIT.ASM`, etc.). Almost certainly a debugger source-search-path directory. See [../../knowledge_base/codebase_history_and_cruft.md](../../knowledge_base/codebase_history_and_cruft.md).

## TILE
Terrain/icon tile-set loading — the tile equivalent of SHAPE's sprite loading. `TILE.H` (`IControl_Type` icon-set header struct, API `Load_Icon_Set`/`Free_Icon_Set`), `ICONSET.CPP`. Contains a dead `#if (0)` block preserving an older incompatible struct version, intentionally kept (commented as matching `WWLIB.I`).

## TIMER
High-resolution/countdown timers for frame pacing and logic-tick counting. `TIMER.H`/`.CPP`, `TIMERINI.CPP`, `TIMERDWN.CPP` (countdown variant).

## WINCOMM
Serial-port/modem comms for null-modem and dial-up multiplayer, replacing the older Greenleaf Comm Library with native Win32 serial APIs (per header comment — see [../../knowledge_base/dependencies_and_missing_libs.md](../../knowledge_base/dependencies_and_missing_libs.md)). `WINCOMM.H`/`.CPP`, `MODEMREG.H`/`.CPP` (AT-command/registry config). Only plain `MAKEFILE` — no `.BOR`/`.WAT`.

## WSA
WSA animation format playback (compressed frame animation, e.g. building-construction animations), XOR-delta frame encoding. `WSA.H` (multiple `Open_Animation`/`Animate_Frame` overloads), `WSA.CPP`. ASM: `XORDELTA.ASM` (`Apply_XOR_Delta`, `Copy_Delta_Buffer`).

## WW_WIN
Small "Windows-mode" text/prompt helper — hide/show cursor around windowed text output, pagination ("more") prompt. `WW_WIN.H`, `WINDOWS.CPP`, `WINHIDE.CPP`. `OLD/` = superseded copy; `WW_WIN.IDE.BAK` = stray backup.

## Toolchain confirmation
Plain `MAKEFILE` (no extension) per module = Watcom (primary/shipped); `MAKEFILE.BOR` = Borland C++; `MAKEFILE.WAT` = explicit alternate Watcom variant; `.IDE` = Borland IDE project files (binary, confirmed via header bytes and internal `C:\BC\INCLUDE\...` paths). Root `PROJECT.CFG`/`MAKE_LIB.MAK`/`MAKE_EXE.MAK` orchestrate the Watcom build across all sub-libraries. No third toolchain evidence (`MEM/MSVC/` despite its name still ships `.BOR`/`.WAT` makefiles, not real MSVC project files).

## Dead code / cruft flagged
6 stray `.BAK` files (`DRAWBUFF/TEST/TEST.BAK`, 4x under `KEYBOARD/OLD/`, `WW_WIN/WW_WIN.IDE.BAK`); superseded/duplicate module snapshots (`AUDIO/OLD/`, `KEYBOARD/OLD/`+`OLDTEST/`+`TEST/`, `MEM/OLDMEM/`+`MSVC/`, `WW_WIN/OLD/`, `PROFILE/UTIL/`); `TILE/TILE.H`'s dead `#if (0)` block; `SRCDEBUG/` as a whole. No significant TODO/FIXME engineering-debt markers found.
