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
| `Buffer_Clear`, `Buffer_Get_Pixel`, `Buffer_Put_Pixel`, `Linear_Blit_To_Linear` (all via `Check_Overlapped_Blit_Capability`), `Init_Present_Device`, **`Present_VisiblePage` + `Update_Palette_Texture` (the main menu is on screen - verified by the owner's eyes and by comparing the displayed indices with an independent decode)**, `LCW_Uncompress` (bit-exact vs a Python reference), the PCX decoder, `Buffer_Print` (menu text renders), the mouse-cursor shape lookup (`Init_Mouse` passes), `Buffer_To_Page`/`Fill_Rect`-style menu drawing (partly) | `Set_Font_Palette_Range`, `Buffer_Draw_Stamp*`/`Cache_Copy_Icon`, `Buffer_Frame_To_Page` (~3900-line ASM rewritten, 4 call sites), `Linear_Scale_To_Linear`, `Buffer_Remap`, `Buffer_Fill_Rect`, `Buffer_Draw_Line`, `Buffer_To_Page`/`To_Buffer`, mouse cursor (`WWMOUSE.CPP`), `LCW_Comp`, XOR-delta, ADPCM (`AUDUNCMP.CPP`), `Build_Fading_Table`, 2x interpolators (`WINASM.CPP`), `Detect_MMX_Availability` |

`SOSCODEC.CPP`'s IMA-ADPCM decode is **stubbed to silence** on purpose (two 89x16 lookup tables not transcribed); `SCOMP_WESTWOOD` (the common type) is real.

## 1. `long` is 32 bits on Win64 (LLP64) - pointer truncation
The code stores pointers in `long`/`unsigned long`/`int` everywhere. `GraphicViewPortClass::Offset` was one (fixed: now `INT_PTR`). **The compiler still lists the rest as C4311/C4312 warnings** - treat that list as a crash forecast:
`cmake --build build/Main --target redalert_code --config Debug -- -m 2>&1 | grep -E "C4311|C4312|C4302"`
Known live ones (`CODE/2KEYFRAM.CPP` is now FIXED - see #12):  `CODE/CONQUER.CPP` 2609/3318/3466/3489/3583, `CODE/IOMAP.CPP` + `IOOBJ.CPP` + `SAVELOAD.CPP` (`TARGET` -> object pointer casts), `CODE/TRIGTYPE.CPP` 1895/1901, `CODE/WOL_GSUP.CPP`, `WIN32LIB/FONT/LOADFONT.CPP:103`, `WIN32LIB/INCLUDE/mono.h:147`, `WIN32LIB/TILE/ICONSET.CPP:214`. Anything that serializes a struct containing pointers (save games!) now has a different size/layout - do not trust save/load until audited.

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

## 12. Pointers hidden in 32-bit "offset" slots (`2KEYFRAM.CPP`) - FIXED
The big-shape cache stores *offsets from the buffer base* in `char *` slots (`KeyFrameSlots`, `ShapeHeaderType::shape_data`) and rebuilt pointers with `(long)`/`(unsigned)` casts. On x64: (a) `memset(slots, 0, frames*4)` cleared only half of each 8-byte slot, so garbage looked like "already built" and `Get_Shape_Header_Data` dereferenced a wild pointer; (b) every `(unsigned)`/`(long)` cast of a buffer base truncated it; (c) `Build_Frame` returned a pointer as `unsigned long` (now `uintptr_t`, also `shape_pointer` in `CONQUER.CPP`). Fixed with `uintptr_t` arithmetic and `sizeof(char*)`. Same idiom may exist elsewhere: grep `(unsigned)`/`(long)` casts of pointers and `*4`-style size literals next to pointer arrays.

## 13. Palette scale: RGBClass is 6-bit, `Red_Component()` is 8-bit - FIXED
VGA DAC values are 0..63. `RGBClass` stores them raw; `Red_Component()` scales to 0..255. `Set_Palette` / `Set_Palette_Range` / `Set_DD_Palette` (which shifts left by 2 itself) take the **raw 6-bit** values. `PaletteClass::Set` passed `Red_Component()`: `(68<<2)&255 == 16`, so every colour was garbled (displayed palette had 25 distinct byte values instead of the PCX's 256-entry ramp). Fixed in `CODE/PALETTE.CPP`. Also `Read_PCX_File` shifted `char` palette bytes with `>>= 2` (sign-extends >=128 -> 0xE0..0xFF; real VGA masked that, `RGBClass`/the fade code do not): now an unsigned shift. Watcom `/j` meant signed `char` here, so `char` is signed in the original too - but hardware quietly masked what MSVC code now computes with.

## 14. Uninitialized locals the Watcom build never tripped over
`BOOL display;` in `WWMessageBox::Process` (`MSGBOX.CPP`) was read before assignment; MSVC's `/RTC` (Debug) turns that into a hard `_RTC_UninitUse` break. Treat RTC-uninit hits as real bugs (initialize to the value the original author evidently expected).

## 15. Measurement pitfalls (cost real time)
- A temporary debug dump that `fopen`s a file can *itself* hang the game: a failed `fopen` -> `fwrite(NULL)` -> CRT assertion message box, and the game then appears stuck "before the title screen". Always NULL-check, overwrite one file instead of numbering, and remove the dump (`grep -rn TEMPDUMP`) before committing.
- Verify data stage by stage against an independent reference (a 20-line Python decoder for LCW/PCX): compressed bytes -> decoded indices -> displayed indices -> palette. That bisected "picture looks cursed" to the palette in minutes after the decoders were proven bit-exact.
- The desktop can be locked/hidden: a screenshot of the lock screen is not evidence about the game. Dump the game's own buffers instead.

## 16. Mouse: coordinates, clip rectangle, and nothing presents the cursor - FIXED
Three independent bugs made the menu "impossible to interact with":
- **Scale.** The game runs at 640x400 but `MainWindow` is a borderless popup over the whole desktop (e.g. 1920x1080) and DXGI stretches the back buffer. `GetCursorPos()` (software cursor, `Get_Mouse_X/Y`) and the `lParam` of `WM_*BUTTON*` (`CODE/KEY.CPP`, `WWKeyboardClass::Message_Handler`) are window pixels. They now go through `Get_Game_Cursor_Pos` / `Client_To_Game_Coords` (`WIN32LIB/MISC/PRESENT_D3D.CPP`).
- **`ClipCursor`.** `WWMouseClass::Set_Cursor_Clip` locked the real pointer to `0,0,Width,Height` = the top-left 640x400 *desktop pixels*. It now clips to the window's client area. Symptom: cursor "trapped top-left"; also makes scripted `SetCursorPos` tests land on (639,399).
- **No present.** The software cursor is drawn into `VisiblePage` from a `timeSetEvent` thread, outside any `Blit()`, and `Present_VisiblePage` only ran from `Blit`. `WWKeyboardClass::Fill_Buffer_From_System` (polled in every loop) now calls `Present_VisiblePage_Throttled()` (~60 Hz, main thread only because the D3D context is not thread safe).
Editing note: the *active* keyboard class is `CODE/KEY.CPP` (and `WIN32LIB/KEYBOARD/MOUSE.CPP`); `CODE/KEYBOARD.CPP` and `WIN32LIB/KEYBOARD/KEYBOARD.CPP`'s `WWKeyboardClass` are not what runs - check `CODE/CMakeLists.txt` and set a breakpoint before editing a same-named file.
The main menu has **no hover highlight** by design (`TextButtonClass` only knows pressed / keyboard-selected via `curbutton`, `MENUS.CPP`); do not treat that as a bug.

## 17. `bool` that held a state, not a flag (`ReadyToQuit`) - FIXED
Watcom 10 had no real `bool`; `CODE/GLOBALS.CPP` declared `bool ReadyToQuit` but stored 1/2/3. With MSVC `ReadyToQuit = 2` stores `true` (1), so `do { Keyboard->Check(); } while (ReadyToQuit == 1);` in `STARTUP.CPP` (clean exit) and `WINSTUB.CPP` never ended: "Exit Game" left a black, busy window forever. Now `int`. Look for other multi-valued `bool` globals (grep `extern bool` in `EXTERNS.H` and compare against assignments of values >1).

## 18. Writing into string literals (read-only on MSVC) - first hit FIXED
Watcom put string literals in writable memory; MSVC puts them in `.rdata`, so `*(name+4) = '1';` on a `char *` that points at a literal is an access violation. First hit: `SidebarClass::Reload_Sidebar` (`SIDEBAR.CPP`, `sidebarnames[]`), which crashed right after "Start New Game" - now works on a local copy. Second hit: `MixFileClass::Offset` (`MIXFILE.CPP`) did `strupr((char *)filename)` on the caller's string, and every `MFCD::Retrieve("literal")` goes through it (found via `Reload_LogoShapes`); it now upper-cases a local copy. Expect more: grep for `char *` locals initialised from literal tables that are later modified (`strcpy`/index assignment/`strupr` on them).
Cheap way to resolve a crash that the user hit without attaching a debugger: take `Fehleroffset` from the Windows event log (`Application Error`, Get-WinEvent), but FIRST check `Fehlerhafter Modulname`: the offset is relative to that module. A fault inside `ucrtbased.dll` (here `strupr` on a literal) was once mis-resolved as a `redalert.exe` offset and named the wrong function; for a CRT/system fault use `runtest.ps1 debug` instead. Then `cdb -cf resolve.cdb cmd.exe /c exit` with `.reload /f redalert.exe=0x7ff700000000,<SizeOfImage>` and `ln`/`lsa` on `base+offset`.

## 19. Stack smashed by an index that was a byte offset once - FIXED (SIDEBAR.CPP)
Symptom: crash at the closing `}` of an innocent function, return address `0x3f3f3f3f3f3f3f3f` ('?????'), stack full of `0x3f`. Cause: `memset(&pal[CYCLE_COLOR_START*3], 0x3f, ...)` on a `PaletteClass`; `operator[]` indexes 3-byte `RGBClass` entries, so `*3` ran 32 entries past the end. It was a raw byte palette once (see the commented-out `GamePalette[...]` lines) and the original silently overran. Lesson: a crash in a function *epilogue* with a repeated-byte stack is a local-buffer overrun in that function or an inlined callee; look for `memset`/`memcpy` on class types with an `operator[]`.

## 20. Enum size: Watcom made enums as small as their values allow - SYSTEMIC, first instance FIXED
`CODE/MAKEFILE` has no `/ei`, so Watcom stored every `enum` in the smallest integer that holds its values (1 byte for most, 2 for `TemplateType` because `TEMPLATE_NONE = 65535`). MSVC always uses 4 bytes unless the enum has a fixed underlying type. Anything that serialises an enum by `sizeof` or stores it inside a struct that is read from disk / sent over the network / saved is therefore wrong. First hit: `MapClass::Read_Binary` read `sizeof(cellptr->TType)` = 4 bytes per cell instead of 2, mis-decoded the whole map and crashed in `TemplateTypeClass::As_Reference` (`CELL.CPP:560`, wild index). Fixed by `enum TemplateType : unsigned short` in `DEFINES.H`.
**Applied so far** with `scripts/size_enums.py` (evaluates each enum's value range and inserts Watcom's underlying type; dry run by default, `--apply` rewrites): all 71 named enums in `DEFINES.H` plus `FACE.H` (`DirType`, whose `DEFINES.H` twin sits in a dead `#ifdef NEVER`), `EVENT.H`, `TACTION.H`, `TEVENT.H`, `TEAMTYPE.H`, `TRIGTYPE.H`, `SESSION.H`, `VORTEX.H`, `FLY.H`, `DRIVE.H`, `IPXGCONN.H`, `IPXCONN.H`, `CONNECT.H`. `static_assert`s at the end of `DEFINES.H` pin the critical sizes. Deliberately *not* touched: anonymous constant enums and dialog/button-ID enums in the other headers (not persisted), and enums the script reports as SKIP (values defined through macros/other enums, e.g. `KeyNumType`, `VersionEnum`, `MapEnum`). Second hit that proved the class: `OverlayClass::Read_INI` read `sizeof(OverlayType)` = 4 bytes per cell from `OverlayPack` (1 byte on disk).
**Original audit TODO (still open for the rest):** list every enum used as a member of a struct/class that is persisted or sent (`CellClass` and the `Save`/`Load` code, `EventClass`, `IControl_Type`, packets in `IPXMGR`/`SESSION`, `.INI`-binary blobs, `sizeof(...)` passed to `Get`/`Put`/`Read`/`Write`). For each, compute Watcom's size (smallest of unsigned char / signed char / unsigned short / short / int that fits the min..max value) and give the enum that underlying type, or read/write an explicit fixed-width integer instead of `sizeof(enum)`.
