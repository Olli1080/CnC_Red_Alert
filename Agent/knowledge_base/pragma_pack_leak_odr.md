---
name: struct-packing-zp1-and-pragma-pack
description: The single most important systemic porting bug - Westwood built CODE/ with /zp1 (byte-packed structs); MSVC defaults to 8. Covers the verified pragma-pack ODR crash, the verified MIX header bug, the predicted .SHP header bug, and the audit method
metadata:
  type: knowledge_base
---

# Struct packing: the original build used `/zp1`, MSVC does not

> File name kept as `pragma_pack_leak_odr.md` because source-header comments already point here.

**Root fact:** `CODE/MAKEFILE:113` says `CC_CFG += /zp1   # Pack structures on byte boundary.` - the shipped game compiled **every struct in `CODE/` byte-packed**. The modern build uses MSVC's default (`/Zp8`, natural alignment). Every struct that is (a) read/written to disk, (b) sent over the network, or (c) shared with ASM/other TUs that assumed packing is now **the wrong size** unless packed explicitly. This is one *class* of bug with (so far) three instances, two verified and one predicted. Expect more.

**Why not just add `/Zp1` to the CMake targets?** Because `CODE/` and `WIN32LIB/` translation units include `<windows.h>`, `<ddraw.h>`, `<dsound.h>` and our own `<d3d11.h>` code; modern Windows/DirectX headers assume default packing (and some `static_assert` on it - that is what broke two audio files in sub-milestone 1a). Byte-packing pointer-bearing OS structs (`DDSURFACEDESC`, `D3D11_*_DESC`) silently corrupts API calls. The workable approach is **explicit `#pragma pack(push,1)` ... `pop` around the specific game structs that need it**, plus a `static_assert(sizeof(...) == N)` for each.

---
## Instance 1 (VERIFIED): leaked `#pragma pack` -> `IndexClass` crash in `Init_Keys`
Symptom: `Is_Archive_Same` (`CODE/search.h:455`) dereferenced `Archive == 0xCCCCCCCCCCCCCC00`.

Finding technique (reusable): `dt redalert!INIClass` -> `SectionIndex` at `+0x20`, so `Archive` is at `INIClass+0x38`; break in the constructor, `ba w8 <addr> "u @rip-0x18 L8; dq <addr> L1; g"`. The store instructions were
```
mov byte  ptr [rax+10h],0       ; IsSorted = false
mov qword ptr [rax+11h],0       ; Archive  = 0     <- +0x11: packed layout
```
but the reader was `cmp qword ptr [rax+18h],0` (natural layout, `Archive` at +0x18). Constructor and reader had been compiled in different translation units with **different layouts of the same template** - an ODR violation the linker merges silently.

Cause: `WIN32LIB/INCLUDE/AUDIO.H:48` `#pragma pack(1);` **never restored**; `SOUNDINT.H:75` `pack(1);` and `:273` bare `pack(4);`. `CODE/FUNCTION.H:193` -> `<wwlib32.h>` -> `<audio.h>`, so everything included *after* that in a TU was packed, and whether `search.h`/`ini.h` came before or after depended on each `.CPP`'s include order.

Fix (verified: crash gone, boot advanced): scope with `push, 1` / `pop` in **both copies** (`WIN32LIB/INCLUDE/` and `WIN32LIB/AUDIO/`) of `AUDIO.H` and `SOUNDINT.H` (`SOUNDINT.H`'s trailing `pack(4)` becomes `pack(pop)`). Committed as `e75d5bf`.

Caveat worth knowing: that leak had been *accidentally* byte-packing (part of) many CODE TUs - i.e. accidentally imitating `/zp1`. Removing it is correct, but it means structs that "worked by accident" in some TUs can now break - which is exactly what Instance 2 turned out to be.

## Instance 2 (VERIFIED): `MixFileClass::FileHeader` - MIX archives parsed as garbage
`CODE/MIXFILE.H`: `struct { short count; long size; }` is the on-disk header (2+4 = **6 bytes**). Compiled unpacked it is **8** (`dt` showed `size` at `+0x004`). The constructor (`MIXFILE.CPP:226-230`) read `sizeof(fileheader)` bytes, so `DataSize` and the `SubBlock` table were misaligned; `Retrieve("TEMPERAT.PAL")` returned NULL and `Bootstrap` crashed in `memmove`. Observable symptom before the fix (via a `CreateFileA` logging breakpoint): `REDALERT.MIX` opened, but `LOCAL.MIX`/`HIRES.MIX`/`NCHIRES.MIX` (which live *inside* it) were then requested **from disk** - i.e. the in-archive lookup failed.

Fix (verified: `Bootstrap` passes, `TEMPERAT.PAL` found, boot advances to `Init_Mouse`): `#pragma pack(push,1)` around the struct + `static_assert(sizeof(FileHeader) == 6)`. Committed as `d1db96f` (`CODE/MIXFILE.H`).
(`SubBlock` is three `long`s = 12 bytes either way; the `alternate` struct is two `short`s = 4 bytes either way.)

## Instance 3 (VERIFIED, fixed): `.SHP` headers -> crash in `Init_Mouse`
Crash was `Init_Game` -> `Init_Mouse` -> `Set_Mouse_Cursor` -> `WWMouseClass::Set_Cursor` -> `ASM_Set_Mouse_Cursor` -> `Get_Shape_Uncomp_Size` reading `shp->DataLength` from a wild address. `WIN32LIB/INCLUDE/SHAPE.H` (and the identical `WIN32LIB/SHAPE/SHAPE.H`): `Shape_Type` (26 bytes packed) and `ShapeBlock_Type` (`Offsets[]` at +2 packed, +4 unpacked) were compiled unpacked, so `Extract_Shape` indexed `Offsets[]` from the wrong place.
Fix: `#pragma pack(push,1)`/`pop` around both + `static_assert(sizeof(Shape_Type)==26)` and `sizeof(ShapeBlock_Type)==2`. **Verified by re-running: the crash moved past `Init_Mouse`.** (`VQ/INCLUDE/WWLIB32/SHAPE.H`, `WWFLAT32/**/SHAPE.H` are other copies that are not part of this build.)

## Instance 4 (VERIFIED, fixed): `CompHeaderType` -> `LCW_Uncompress` wrote past the buffer
`WIN32LIB/INCLUDE/IFF.H` (+ `WIN32LIB/IFF/IFF.H`): `struct { char Method; char pad; long Size; short Skip; }` is the 8-byte header in front of every compressed block (`.CPS`, loaded via `Load_Uncompress`/`Uncompress_Data`). Unpacked it is 12 bytes with `Size` at +4 instead of +2, so the "uncompressed size", skip count and the in-place source position were all wrong; `LCW_Uncompress` crashed in `Init_Color_Remaps` -> `Load_Picture("PALETTE.CPS")`. Fix: pack + `static_assert(sizeof == 8)`. After the fix `PALETTE.CPS` decodes **bit-identically** to an independent Python reference decoder (only the last byte differs: harmless in-place overlap with the end marker).

---
## Audit method - find every struct that needs packing
**Still to audit proactively** (four instances found so far, each one only after it crashed): `IControl_Type` (icon sets), font headers (`FONT.H`), `.WSA`, `KeyFrameHeaderType` (`2KEYFRAM`), `.AUD` headers (done), save-game structs, network packets.
1. `grep -rnE "(Read|Get|Write|Put|fread|fwrite)\s*\([^;]*sizeof" CODE WIN32LIB --include=*.CPP --include=*.H` - structs passed to file/stream I/O with `sizeof`.
2. `grep -rn "typedef struct" WIN32LIB/INCLUDE/*.H CODE/*.H` in format-related headers: `SHAPE.H` (.SHP), `MIXFILE.H`, `ICONCACH.H`/`TILE` (`IControl_Type`, `.TMP` icon sets), `FONT.H` (`.FNT`), `WSA.H` (`.WSA` animation headers), `AUDIO.H` (`AUDHeaderType` - already packed), `CODE/2KEYFRAM.CPP`/`2KEYFRAM.H` (`KeyFrameHeaderType`), `CODE/CDATA`/`SAVELOAD.CPP` (save games; also contain pointers - see [x64_porting_pitfalls.md](x64_porting_pitfalls.md) #1), `CODE/EVENT.H`/`PACKET`-style network structs (multiplayer; out of scope for now).
3. For each: find the real on-disk size (from the original tools in `TOOLS/`, the ASM that read it, or the field-by-field comment), then `#pragma pack(push,1)`/`pop` + `static_assert(sizeof(T) == N, ...)`. The `static_assert` is what makes the next agent's life easy - a wrong size becomes a compile error instead of a runtime crash three call layers away.
4. `grep -rn "pragma pack" CODE WIN32LIB/INCLUDE WIN32LIB/*/` and make sure every `pack(N)` has a matching `pop`. Known balanced: `CODE/MEMCHECK.H` (`pack(1)`..`pack()`), `SOS.H`/`SOSDATA.H`/`SOSFNCT.H` (`pack(4)`..`pack()`).
5. Symptom fingerprints: a field read at an offset off by 1-2 bytes; sizes off by the padding; counts/lengths that look byte-swapped or shifted; `dt` in cdb showing padding you don't expect; the same function touching one field at two different offsets.
6. `#pragma option -a1` (Borland) and `#pragma warn` are ignored by MSVC (C4068) - they do **not** pack.
