---
name: x64-porting-pitfalls
description: Consolidated checklist of the bug classes found so far while making this 1990s Watcom/32-bit code run under MSVC x64 - use it to triage the next crash quickly
metadata:
  type: knowledge_base
---

# Porting pitfalls found so far (and how to recognise them)

Every item below was a *real* bug in this repo, not a hypothetical. When the next crash shows up, walk this list before theorising. Per-sub-milestone detail: [../plans_done/](../plans_done/summary.md).

## 0. "It compiles" proves almost nothing here
Sub-milestone 1c rewrote ~66 `.ASM` files as C++ and got them to *link*, but for most of that work **nothing ever executed them** (the game died at the DirectDraw wall first). The first time real pixel code ran (`Buffer_Clear`), it crashed on a wrong-formula bug (#2). Treat every rewritten primitive as **unverified until it has run for real**:

| Actually executed at runtime so far | Never executed yet |
|---|---|
| `Buffer_Clear`, `Buffer_Get_Pixel`, `Buffer_Put_Pixel`, `Linear_Blit_To_Linear` (all via `Check_Overlapped_Blit_Capability`), `Init_Present_Device` (D3D11 device, swapchain, both textures and `D3DCompile` of both shaders succeeded; the `CreateVertexShader`/`CreatePixelShader` calls are unchecked) | `Present_VisiblePage`'s draw path and `Update_Palette_Texture` (nothing has blitted to the visible page yet), `Buffer_Print`/`Set_Font_Palette_Range`, `Buffer_Draw_Stamp*`/`Cache_Copy_Icon`, `Buffer_Frame_To_Page` (~3900-line ASM rewritten, 4 call sites), `Linear_Scale_To_Linear`, `Buffer_Remap`, `Buffer_Fill_Rect`, `Buffer_Draw_Line`, `Buffer_To_Page`/`To_Buffer`, mouse cursor (`WWMOUSE.CPP`), `LCW_Comp`, XOR-delta, ADPCM (`AUDUNCMP.CPP`), `Build_Fading_Table`, 2x interpolators (`WINASM.CPP`), `Detect_MMX_Availability` |

`SOSCODEC.CPP`'s IMA-ADPCM decode is **stubbed to silence** on purpose (two 89x16 lookup tables not transcribed); `SCOMP_WESTWOOD` (the common type) is real.

## 1. `long` is 32 bits on Win64 (LLP64) - pointer truncation
The code stores pointers in `long`/`unsigned long`/`int` everywhere. `GraphicViewPortClass::Offset` was one (fixed: now `INT_PTR`). **The compiler still lists the rest as C4311/C4312 warnings** - treat that list as a crash forecast:
`cmake --build build/Main --target redalert_code --config Debug -- -m 2>&1 | grep -E "C4311|C4312|C4302"`
Known live ones: `CODE/2KEYFRAM.CPP` 433-470, `CODE/CONQUER.CPP` 2609/3318/3466/3489/3583, `CODE/IOMAP.CPP` + `IOOBJ.CPP` + `SAVELOAD.CPP` (`TARGET` -> object pointer casts), `CODE/TRIGTYPE.CPP` 1895/1901, `CODE/WOL_GSUP.CPP`, `WIN32LIB/FONT/LOADFONT.CPP:103`, `WIN32LIB/INCLUDE/mono.h:147`, `WIN32LIB/TILE/ICONSET.CPP:214`. Also `2KEYFRAM.CPP`'s `(unsigned)BigShapeBufferStart`. Anything that serializes a struct containing pointers (save games!) now has a different size/layout - do not trust save/load until audited.

## 2. `Offset` is the *complete* pixel pointer
`Get_Offset()` alone is the address of the first pixel (plain buffer: set to `Buffer` at `Init`; locked DirectDraw surface: the lock pointer; attached sub-viewport: parent's `Offset` + row/col delta). `Get_Buffer()` is only an ownership accessor. Adding them (16 sites in the 1c rewrites, fixed) double-counts. Stride = `Width + XAdd + Pitch`.

## 3. Unbalanced `#pragma pack` = layout differs per translation unit
See [pragma_pack_leak_odr.md](pragma_pack_leak_odr.md). Fingerprint: one function touches a field at a different offset than its siblings.

## 4. Old compilers instantiated whole templates eagerly; MSVC doesn't
`extern Foo<T> dummy;` idioms forced instantiation on Watcom. Fixed with real `template class X<T>;` explicit instantiations (`CCPTR.CPP`, `VECTOR.CPP`, `DYNAVEC.CPP`, `HEAP.CPP`, `CCFILE.H`). A new "unresolved external" on a template member = a missing explicit instantiation.

## 5. Bodyless declaration shadows a working template
`inline E operator|(E,E);` (no body) next to `template<class T> T operator|(T,T){...}` is a *separate never-defined overload* that wins overload resolution. Fixed with `template E operator|(E,E);` (explicit instantiation) in `JSHELL.H`/`DEFINES.H`/`DRIVE.H`/`EDIT.H`/`GADGET.H` (also postfix `++` on 21 enums). Same shape via friend-injection in `INT.H` (`Generate_Prime`).

## 6. Linkage/type mismatches that link fine but bind the wrong symbol
MSVC mangles a global's *type* into its name: `bool` vs `BOOL` globals in different TUs = unresolved. `extern "C"` on one side only = different symbol (`RandNumb`, `UseOldShapeDraw`). When a "symbol exists in the .lib but won't link", compare *linkage*, not spelling.

## 7. Watcom `#pragma aux` inline assembly is silently dropped by MSVC
Bodies compile to nothing. `grep -rn "pragma aux" CODE WIN32LIB` for anything suspicious that "does nothing".

## 8. A no-op `Error()` turns retry loops into infinite loops
`RawFileClass::Open/Read` retried forever once the interactive Retry/Abort dialog became a no-op (fixed). Any `for(;;){ ...; Error(...); continue; }` pattern is suspect if a run *hangs* (CPU spinning, no window) - look for them first.

## 9. Old-compiler-only syntax that MSVC ignores
`#pragma option`, `#pragma warn`, `far`, `_USERENTRY`, `#pragma pack(N);` with trailing `;`. Ignored != harmless when the pragma controlled layout/warnings the code relied on.

## 10. Two live copies of headers
`WIN32LIB/INCLUDE/X.H` and `WIN32LIB/<MODULE>/X.H` (also `CODE/AUDIO.H` etc.) are duplicates; include-path order decides which compiles (1b's biggest single bug: `<x.h>` vs `"x.h"` and `CODE/` listed before `WIN32LIB/INCLUDE`). Edit both, and check `diff` afterwards. `SRCDEBUG/`, `OLD/`, `OLDTEST/`, `TEST/`, `.BAK` are dead copies.

## 11. Use the debug CRT's fill patterns as a free uninitialized-memory detector
Debug builds fill fresh stack with `0xCC`, heap with `0xCD`, freed heap with `0xDD`, guard bytes `0xFD`. Garbage like `0xCCCCCCCCCCCCCC00` in a field means "never (fully) written", not "corrupted by an attacker".
