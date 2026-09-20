---
name: boot-to-main-menu
description: Active plan - iterate redalert.exe from "crashes in Init_Mouse" to "shows the main menu"; ordered next steps, hypotheses, acceptance criteria, and the loop to follow
metadata:
  type: plan
---

# Plan: get `redalert.exe` from the current crash to the main menu

Parent plan: [modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md) (Phases 0-3 done; this is the runtime-bring-up tail of Phase 1/3). Start with [../HANDOFF.md](../HANDOFF.md) for state, tools and rules.

## Goal / definition of done
1. **M1**: `Init_Game()` returns (no crash) - all of `Bootstrap`, `Init_Mouse`, fonts, palette, one-time systems pass.
2. **M2**: the main menu draws non-black, correctly coloured pixels in the D3D11 window (first proof the presenter's *draw path* and palette texture work - neither has ever executed).
3. **M3**: mouse cursor visible and moving; menu buttons respond to input.
4. **M4**: a skirmish/scenario loads and the map renders (`Buffer_Frame_To_Page`, stamps, shapes exercised for real).
5. **M5**: audio (DirectSound) initialises and plays a sound effect.

## The loop (this is 90% of the work)
```
.\scripts\runtest.ps1 build ; .\scripts\runtest.ps1 debug      # -> exception + symbolized stack
   -> identify the first redalert! frame -> read the code -> check knowledge_base/x64_porting_pitfalls.md
   -> fix root cause (not the symptom) -> rebuild -> repeat
```
Write each *class* of bug down in `knowledge_base/x64_porting_pitfalls.md` as you find it (the next crash is very likely the same class as the last).

## Ordered steps
### Step 0 - land the verified working-tree fixes (do this first)
The working tree contains **uncommitted, verified** changes; inspect with `git status` / `git diff`, then commit (suggested split: one commit per bullet):
- `WIN32LIB/{INCLUDE,AUDIO}/{AUDIO,SOUNDINT}.H` - scope `#pragma pack` with push/pop (fixes the `IndexClass` crash; see [../knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md)).
- `CODE/MIXFILE.H` - `#pragma pack(push,1)` + `static_assert(sizeof==6)` on `FileHeader` (fixes `Bootstrap`'s NULL `Retrieve`).
- `WIN32LIB/MISC/DDRAW.CPP` - `Set_DD_Palette` now fills `PaletteEntries[]` unconditionally (previously gated on `PaletteSurface`, which is always NULL now => the D3D11 palette texture would have stayed zero => permanently black screen) and `Wait_Blit` returns early when `PaletteSurface` is NULL (was a NULL deref via `CODE/INTERPAL.CPP:358`). *Reasoned and builds; not yet exercised at runtime* (needs the first palette set / first blit).
- `scripts/runtest.ps1` + the `Agent/` docs.

### Step 1 - current blocker: `Init_Mouse` -> `Get_Shape_Uncomp_Size` (predicted: `.SHP` header packing)
Full evidence and the predicted fix in [../knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md) "Instance 3". Pack `Shape_Type` and `ShapeBlock_Type` in `WIN32LIB/INCLUDE/SHAPE.H` (and the `WIN32LIB/SHAPE/` copy if present) with `static_assert`s, rebuild, rerun. **Then run the audit in that document (steps 1-2) proactively for all other on-disk structs** rather than waiting for each to crash: `IControl_Type` (icon sets), font headers, `.WSA`, `2KEYFRAM` key-frame headers, palette/CPS loaders.
Caveat: `ASM_Set_Mouse_Cursor` (`WIN32LIB/KEYBOARD/WWMOUSE.CPP`) is one of the 1c rewrites that has *never run*; once the header packing is fixed, re-read it against `WIN32LIB/KEYBOARD/WWMOUSE.ASM` if the cursor draws wrong.

### Step 2 - the never-executed 1c primitives will fail in their own ways
Order of first execution is roughly: mouse cursor -> fonts (`Buffer_Print`, `Set_Font_Palette_Range`) -> palette (`Set_DD_Palette`, `Build_Fading_Table`, PaletteClass) -> `Buffer_To_Page`/`To_Buffer`/`Fill_Rect`/`Draw_Line`/`Remap` -> stamps/icons (`Buffer_Draw_Stamp*`) -> shapes (`Buffer_Frame_To_Page`, `Linear_Scale_To_Linear`). When one crashes or draws garbage, re-derive its behaviour from the original ASM next to it (`WIN32LIB/DRAWBUFF/*.ASM`, `CODE/2KEYFBUF.ASM`) - all originals are still in the tree. Table of which are proven vs unproven: [../knowledge_base/x64_porting_pitfalls.md](../knowledge_base/x64_porting_pitfalls.md) #0.

### Step 3 - first visible frame (M2): checklist if the window stays black
1. Is `Present_VisiblePage` being reached? Breakpoint `redalert!Present_VisiblePage`. It only fires from the three `GraphicViewPortClass::Blit` overloads in `WIN32LIB/{INCLUDE,DRAWBUFF}/GBUFFER.H` when `dest.GraphicBuff == VisibleGraphicBuffer`. Direct pixel writes (`Put_Pixel`, text, the mouse cursor) do **not** trigger a present - only a later `Blit` does.
2. Is `VisibleGraphicBuffer` set? It is assigned in `GraphicBufferClass::Init` when `flags & GBC_VISIBLE`. `CODE/STARTUP.CPP` has the *live* `InitDDraw` logic **inlined in `WinMain`** (the `#else` of `#ifdef MPEGMOVIE`); the standalone `InitDDraw()` is dead code in this build.
3. Palette: `Update_Palette_Texture` runs from `Set_DD_Palette` (the D3D device already exists by then - it is created in `Set_Video_Mode`). Check `PaletteEntries[]` is non-zero after the first palette set, and that the index-0 colour isn't just legitimately black.
4. `Map`/`Unmap` HRESULTs in `PRESENT_D3D.CPP` are unchecked; use the D3D11 debug layer or add a `MessageBox` on failure. `CreateVertexShader`/`CreatePixelShader` results are also unchecked (only `D3DCompile` is).
5. The window is a fixed `WS_POPUP` covering the whole screen while the swapchain is created at `ScreenWidth x ScreenHeight` (640x400, or 640x480 fallback) from `Set_Video_Mode`'s arguments, so DXGI/DWM should stretch it (ugly aspect ratio, but fine for now - unverified).

### Step 4 - known deferrals (do not chase until M1-M3 done)
Alt-tab/focus/device-loss; window resize; movies (VQA/MPEG stubbed); WOL/modem multiplayer (excluded); the IMA-ADPCM (`SCOMP_SOS`) audio codec (silence stub); save-game struct layouts with 32-bit pointers; the remaining C4311/C4312 truncation warnings ([../knowledge_base/x64_porting_pitfalls.md](../knowledge_base/x64_porting_pitfalls.md) #1) - fix as they bite, but skim them now for anything on the boot path.

## Guardrails (also in HANDOFF.md)
- Only the owner's own game data; never download or fetch third-party binaries (a DirectDraw shim, cracks, asset packs...) - the D3D11 presenter exists precisely so none is needed.
- Don't hard-link `REDALERT.INI` (the game rewrites it; that once modified the owner's Steam file). Use the script.
- Keep duplicate headers (`WIN32LIB/INCLUDE/X.H` vs `WIN32LIB/<MODULE>/X.H`) in sync; `SRCDEBUG/`, `OLD*/`, `*.BAK` are dead.
- Verify by *running*, not by compiling - see the pitfalls doc for why "it links" was misleading for 1c.
