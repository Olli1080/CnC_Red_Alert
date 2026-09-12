---
name: wwflat32
description: The DOS-extender predecessor support library WIN32LIB was ported from — one section per subdirectory
metadata:
  type: project_state
---

# WWFLAT32/ — DOS-Extender Support Library

Parent index: [summary.md](summary.md). This is the earlier generation of [win32lib.md](win32lib.md) — full relationship explained in [../../knowledge_base/codebase_history_and_cruft.md](../../knowledge_base/codebase_history_and_cruft.md).

## Overview
Westwood's DOS-targeted "flat 32-bit" support library, compiled to run under a DOS extender (comments name "RATIONAL SYSTEM DOS4GW" explicitly), giving 32-bit flat addressing on top of real-mode DOS. Provides: file I/O with a resident-file cache, an in-house malloc replacement, audio (SOS/HMI driver + in-house ADPCM-style decompression), a class-based 2D graphics stack (`GraphicBufferClass`/`GraphicViewPortClass`) with pluggable MCGA (mode 13h linear) vs. SVGA/VESA (bank-switched) backends, sprite/tile blitters, WSA animation, IFF/PCX/LBM loading, LCW compression, palette/font routines, keyboard/mouse via real-mode interrupt hooking, CD audio, and DOS descriptor/selector management bridging real- and protected-mode code. Watcom-only (no `.BOR` files anywhere in this tree — contrast WIN32LIB's dual Watcom+Borland setup). Shares authorship and near-identical algorithm files with WIN32LIB (some literally byte-identical, e.g. `AUDIO/SOUNDIO.CPP` in both trees shares the same RCS `$Id` tag).

## WWFLAT32 vs. WIN32LIB — key divergence points
WWFLAT32 depends on `<dos.h>`/`<bios.h>`, real-mode segment:selector pairs (`SEGSEL{seg,sel}`), DPMI real-mode callback plumbing (`DPMI_real_alloc/free/intr/call`), a dedicated `DESCMGMT` selector-fixup module, and a keyboard/mouse subsystem split into real-mode (`KEYIREAL.ASM`) vs. protected-mode (`KEYIPROT.ASM`) interrupt handlers with a page-fault handler (`PAGFAULT.ASM`) — all classic DOS4GW concerns. WIN32LIB's equivalents are Win32-native: a plain `WWKeyboardClass` fed by Windows messages, no selector fixups needed (flat Win32 memory), DirectDraw instead of MCGA/VESA mode-setting. WWFLAT32-only directories: `DESCMGMT`, `MCGAPRIM`, `SVGAPRIM`, `VIDEO`, `WINDOWS`. WIN32LIB-only directories: `DRAWBUFF`, `WINCOMM`, `WW_WIN`, `MOVIE`, `PROFILE`, `RAWFILE`.

## AUDIO
SOS-driver-based sound with custom ADPCM-like codec. `SOUND.H`/`SOUNDINT.H`/`.CPP` (playback API/state), `SOUNDIO.CPP` (streaming I/O, shares RCS tag with WIN32LIB's copy), `SOUNDLCK.CPP` (locks DMA-driver memory). ASM: `SOSCODEC.ASM`, `AUDUNCMP.ASM` + lookup tables. `NEW/` subfolder = parallel/alternate copy (in-progress revision kept alongside); several `.BAK` files present.

## DESCMGMT
DOS extender/DPMI selector plumbing — GDT/LDT selector allocation/fixup so protected-mode code can address specific real-mode/physical memory (screen, PSP, environment block, FPU-vendor selectors). Its core `Map_Segment_To_Address` has its "real" implementation commented out with a note it's "no longer needed by RATIONAL SYSTEM DOS4GW linear addressing mode" — **largely vestigial**, kept for selector bookkeeping. `DESCMGMT.H`/`.CPP`, `FFIXSEL.ASM`, `FGETCS.ASM`/`FGETDS.ASM`/`FGETSEL.ASM`. **No WIN32LIB equivalent** — eliminated in the Win32 port.

## DIPTHONG
Same digram text-compression codec as WIN32LIB's copy (`DIPTHONG.CPP`/`.H`, `_DIPTABL.CPP`), near-identical.

## EXAMPLE
Skeleton template app for linking against the library — same role as WIN32LIB's `EXAMPLE/`. `MAIN.CPP`, `STARTUP.CPP`, `WWLIB32.H`.

## FILE
Custom DOS file I/O with a resident-file cache system (handle-table abstraction, "packed"/mixed-archive file support, preload/flush policies) — **this module has no WIN32LIB counterpart** (WIN32LIB's equivalent role is split across `RAWFILE/` there). `FILE.H`/`.CPP`/`FILEIO.CPP`/`FILELIB.CPP`, `FILEINIT.CPP` (`WWDOS_Init`/`Shutdown`), `FILECACH.CPP` (`Make_File_Resident`, `Flush_Unused_File_Cache`), `FILECHNG.CPP`/`FILEINFO.CPP`/`FILEDATA.CPP`/`FILEGLOB.CPP`/`FINDFILE.CPP`/`FGLOB2.CPP`. ASM: `FFIRST.ASM`, `DEVICES.ASM`/`DEVTABLE.ASM`, `HARDERR.ASM` (INT 24h critical-error handler). Only surviving copy of this module's source outside `WIN32LIB/SRCDEBUG/`'s flattened remnants.

## FONT
Bitmap font rendering, same role as WIN32LIB's copy. `FONT.H`/`.CPP`, `LOADFONT.CPP`, `SET_FONT.CPP`. ASM: `SETFPAL.ASM`, `TEXTPRNT.ASM`.

## IFF
Image loading (PCX/LBM/IFF) + LCW compression, same role as WIN32LIB's copy. `IFF.H`/`.CPP` (`IFFize_WORD/LONG` byte-swap macros for big-endian IFF chunks), `LOADPCX.CPP`/`WRITEPCX.CPP`, `LOAD.CPP`/`LOADPICT.CPP`/`WRITELBM.CPP`. ASM: `LCWCOMP.ASM`/`LCWUNCMP.ASM`, `PACK2PLN.ASM`.

## INCLUDE
Flattened shared header tree (mirrors WIN32LIB's `INCLUDE/` role). `WWSTD.H` — the umbrella base-types header (`REALPTR = unsigned long`, `VOID`/`BOOL`/`IBM`/`AMIGA` platform macros), included nearly everywhere. Several `.BAK` files here suggest this header cache drifted/was hand-patched independently of per-module source at some point.

## KEYBOARD
Real-mode INT-9 hardware keyboard interrupt hooking — the biggest architectural contrast point with WIN32LIB's message-driven `WWKeyboardClass`. `KEYBOARD.H` (scancode enums, mouse prototypes), `KEYIREAL.ASM` (real-mode ISR), `KEYIPROT.ASM` (protected-mode dispatch), `PAGFAULT.ASM` (page-fault handler needed because DOS4GW demand-paged memory can fault inside an interrupt), `KEYBOARD.ASM` (glue), `MOUSE.ASM` (INT 33h wrapper). Heavy cruft: `NEW/` and `X/` subfolders hold alternate/parallel full copies; 5+ `.BAK` files.

## LIB
Not source — holds one prebuilt static library, `SOSDW1PS.LIB` (also present under `AUDIO/`).

## MCGAPRIM
**Confirmed**: low-level graphics primitives for MCGA/mode-13h (320×200×256-color linear framebuffer, no bank switching). Supplies the concrete `MCGA_*`/`Linear_*` implementations `GBUFFER.H`'s classes call through function pointers. Design: default library ships no-op stubs; linking `MCGAPRIM` or `SVGAPRIM` overrides them at link time so a program only pays for the backend it needs. `MCGAPRIM.H`/`.INC`, `GBUFFER.H`/`.CPP` (class hierarchy + MCGA impl). ASM: `BITBLIT.ASM`, `PUTPIX.ASM`/`GETPIX.ASM`, `CLEAR.ASM`, `FILLRECT.ASM`/`FILLQUAD.ASM`, `DRAWLINE.ASM`, `STAMP.ASM` (tile drawing), `REMAP.ASM` (house-color recolor), `SCALE.ASM`, `TXTPRNT.ASM`, plus `V`-prefixed VESA-tagged variants of most of these.

## MEM
Custom heap allocator with LRU-style aging/GC (critical for DOS-era 640KB/extended-memory budgets). `MEM.CPP`/`WWMEM.H` (`Mem_Alloc`, `Mem_Find_Oldest`, `Mem_Cleanup`, `Mem_Lock_Block`), `ALLOC.CPP`, `NEWDEL.CPP`. ASM: `MEM_COPY.ASM`. `OLD/` subfolder = prior revision.

## MISC
CPU/OS detection, RNG, CRC, argv parsing, DOS-interrupt helpers. `MISC.H`. ASM: `DETPROC.ASM`/`PROC.ASM`/`OPSYS.ASM`, `RANDOM.ASM`, `CRC.ASM`, `FACING8/16/FF.ASM`, `FADING.ASM`, `CLIPRECT.ASM`, `SHAKESCR.ASM`, `REVERSE.ASM`. `KEYCODE.CPP` (keycode table), `EXIT.CPP`/`VERSION.CPP`/`FINDARGV.CPP`/`DELAY.CPP`.

## MONO
Debug output to a secondary monochrome monitor (genuine dual-monitor DOS-era debugging). `MONO.H`, `MONO.ASM` (direct writes to segment B000h), `INITMONO.CPP`, `MONOPRNT.CPP`.

## PALETTE
VGA palette load/set/morph-fade. `PALETTE.H`/`.CPP`, `LOADPAL.CPP`, `MORPHPAL.CPP`. ASM: `PAL.ASM` (`Set_Palette_Range` direct VGA DAC I/O).

## PLAYCD
Red Book CD audio via DPMI real-mode calls to the BIOS/MSCDEX interrupt. `PLAYCD.H` (`SEGSEL` struct, `DPMI_real_alloc/free/intr/call` externs — direct evidence of the real/protected-mode bridging pattern), `PLAYCD.ASM`, `REDBOOK.CPP`, `GETCD.CPP`.

## SHAPE
Same hand-tuned-per-variant sprite drawing system as WIN32LIB's copy (including the identical `DS_*.ASM` family). `SHAPE.H`, `DRAWSHP.ASM`, `GETSHAPE.CPP`, `SETSHAPE.ASM`, `PRIOINIT.CPP`.

## SRCDEBUG
Staging directory of byte-identical copies of the most delicate low-level files (`MEM/`'s `ALLOC.CPP`/`MEM.CPP`/`MEM_COPY.ASM`/`NEWDEL.CPP`; `KEYBOARD/`'s ISR files) — likely a special debug-build target for the hardest-to-debug code (memory manager, interrupt handlers) on real DOS hardware. No makefile of its own.

## SVGAPRIM
**Confirmed**: low-level graphics primitives for VESA/SVGA modes beyond MCGA's fixed 320×200 (640×400 up to 1024×768). Manages **bank switching** (`BankTable`, `WinGranularity`/`WinSize`, `Vesa_Asm_Set_Win`/`Next_Win`) since VESA framebuffers exceed one 64KB real-mode segment. `SVGAPRIM.INC` (cross-references MCGAPRIM's `VGETPIX.ASM` module). ASM: `VESA.ASM` (bank-switch calls), `VGETPIX.ASM`/`VPUTPIX.ASM`, `VCLEAR.ASM`, `VTOBUFF.ASM`/`VTOPAGE.ASM`, `VTXTPRNT.ASM`, `VVBLIT.ASM`, `VLBTOVE.ASM`/`VVETOLB.ASM` (linear-buffer ↔ VESA cross-mode blit converters), `VSCLTOVE.ASM`/`VVETOSCL.CPP` (scaled cross-mode transfer). Together with MCGAPRIM implements exactly the two DOS-era video-mode strategies unified behind one function-pointer interface.

## TILE
Terrain/map tile ("icon set") loading on top of MCGAPRIM's stamp-drawing. `TILE.H` (`Load_Icon_Set` API), `ICONSET.CPP`.

## TIMER
Frame-timing (`TimerClass` wrapping a hardware PIT tick counter). `TIMER.H`/`.CPP`, `TIMERINI.CPP`, `TIMERDWN.CPP`. ASM: `TIMERA.ASM`/`TIMEREAL.ASM` (system-timer interrupt hook, akin to KEYBOARD's real-mode hook).

## VIDEO
Video-mode management: mode switching, VESA capability querying, vertical-blank sync — decides which of MCGAPRIM/SVGAPRIM to activate. `VIDEO.H`/`.CPP` (mode constants, `VesaInfoType`/`VesaModeInfoType` structs matching the VESA BIOS extension spec). ASM: `VESAHOOK.ASM`/`VESAREAL.ASM` (real-mode VESA BIOS trampolines, another DPMI bridge like KEYBOARD/PLAYCD), `VERTBLNK.ASM`. `INITDLAY.CPP` calibrates the busy-wait delay loop to CPU speed.

## WINDOWS
DOS **text-mode** "window" (viewport) management for bordered scrollable text regions — not GUI windows. `WINDOW.H`/`WINDOWS.H`/`.CPP` (`WindowList[][8]` array, `Change_Window`, "more" prompt paging), `WINHIDE.CPP` (scoped mouse-cursor hide/show). No WIN32LIB equivalent — superseded by native Win32 windowing.

## WSA
Same `.WSA` XOR-delta animation playback as WIN32LIB's copy. `WSA.H`/`.CPP`, ASM `XORDELTA.ASM`.

## Build/toolchain and dead-file summary
Watcom-only (`wcc`/`wlib`, per-directory `MAKEFILE` + root `MAKE_EXE.MAK`/`MAKE_LIB.MAK`/`PROJECT.CFG`); TASM `IDEAL`-mode assembly. Required env vars: `WWFLAT`, `WWVCS`, `WATCOM`. 43 `.BAK` files scattered across most subdirectories; `AUDIO/NEW/`, `KEYBOARD/NEW/`+`KEYBOARD/X/`, `MEM/OLD/` are full parallel subtree copies roughly double-counting files in those three directories — see [../../knowledge_base/codebase_history_and_cruft.md](../../knowledge_base/codebase_history_and_cruft.md).
