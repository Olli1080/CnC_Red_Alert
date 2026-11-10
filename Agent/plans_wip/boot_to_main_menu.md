---
name: boot-to-main-menu
description: Active plan - redalert.exe boots, the menu works and mission 1 plus the campaign map are playable (M1-M4 done); M5 audio is half done (music yes, sound effects missing); next steps and the loop to follow
metadata:
  type: plan
---

# Plan: get `redalert.exe` from the current crash to the main menu

Parent plan: [modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md) (Phases 0-3 done; this is the runtime-bring-up tail of Phase 1/3). Start with [../HANDOFF.md](../HANDOFF.md) for state, tools and rules.

## Status (2026-11-10): M1-M4 DONE, M5 half done
`Init_Game` returns, the main menu renders and is operable, Start New Game (Allies) loads mission 1 which is playable to the end (map, units, sidebar, colours, music, scroll speed confirmed by the owner), followed by the score screen and the campaign map animation. **Open: sound effects are almost entirely missing (owner, 2026-11-10), movies are skipped, mission 2+ unverified.** The sections below keep the original step-by-step history; the live to-do list is 'Next concrete steps' at the end.

## Goal / definition of done
1. **M1**: `Init_Game()` returns (no crash) - all of `Bootstrap`, `Init_Mouse`, fonts, palette, one-time systems pass.
2. **M2**: the main menu draws non-black, correctly coloured pixels in the D3D11 window (first proof the presenter's *draw path* and palette texture work - neither has ever executed).
3. **M3** (DONE): mouse cursor visible and moving; menu buttons respond to input.
4. **M4** (DONE): a scenario loads and the map renders (`Buffer_Frame_To_Page`, stamps, shapes exercised for real); mission 1 plays to the end.
5. **M5** (PARTLY): audio (DirectSound) initialises (done), music plays (done, SOS ADPCM decoder), **sound effects play (open)**.

## The loop (this is 90% of the work)
```
.\scripts\runtest.ps1 build ; .\scripts\runtest.ps1 debug      # -> exception + symbolized stack
   -> identify the first redalert! frame -> read the code -> check knowledge_base/x64_porting_pitfalls.md
   -> fix root cause (not the symptom) -> rebuild -> repeat
```
Write each *class* of bug down in `knowledge_base/x64_porting_pitfalls.md` as you find it (the next crash is very likely the same class as the last).

## Ordered steps
### Step 0 - already done (for reference)
The fixes that got the boot from `Init_Keys` to `Init_Mouse` are committed: `e75d5bf` (pragma pack scoping - [../knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md)), `d1db96f` (MIX `FileHeader` packing), `eacb07d` (`Set_DD_Palette` now fills `PaletteEntries[]` unconditionally - previously gated on `PaletteSurface`, always NULL now, which would have left the D3D11 palette texture zero => permanently black screen - plus a `Wait_Blit` NULL guard; *reasoned and builds, not yet exercised at runtime*), `ef0fee9` (script + docs).

### Step 1 - DONE: `Init_Mouse` -> `Get_Shape_Uncomp_Size` (was `.SHP` header packing)
Fixed and verified (see also instance 4, `CompHeaderType`). Evidence in [../knowledge_base/pragma_pack_leak_odr.md](../knowledge_base/pragma_pack_leak_odr.md) "Instance 3". Pack `Shape_Type` and `ShapeBlock_Type` in `WIN32LIB/INCLUDE/SHAPE.H` (and the `WIN32LIB/SHAPE/` copy if present) with `static_assert`s, rebuild, rerun. **Then run the audit in that document (steps 1-2) proactively for all other on-disk structs** rather than waiting for each to crash: `IControl_Type` (icon sets), font headers, `.WSA`, `2KEYFRAM` key-frame headers, palette/CPS loaders.
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
Alt-tab/focus/device-loss; window resize; movies (VQA/MPEG stubbed - see [movies.md](movies.md)); WOL/modem multiplayer (excluded); save-game struct layouts with 32-bit pointers. (The IMA-ADPCM codec stub and the C4311/C4312 truncation audit are done: pitfalls #22, #23.)

## Guardrails (also in HANDOFF.md)
- Only the owner's own game data; never download or fetch third-party binaries (a DirectDraw shim, cracks, asset packs...) - the D3D11 presenter exists precisely so none is needed.
- Don't hard-link `REDALERT.INI` (the game rewrites it; that once modified the owner's Steam file). Use the script.
- Keep duplicate headers (`WIN32LIB/INCLUDE/X.H` vs `WIN32LIB/<MODULE>/X.H`) in sync; `SRCDEBUG/`, `OLD*/`, `*.BAK` are dead.
- Verify by *running*, not by compiling - see the pitfalls doc for why "it links" was misleading for 1c.

## Next concrete steps (start here)
Done so far: menu + input (#16, #17), scenario load and logic ticks, rendering and colours (#21), enum/literal/pointer audits (#18-#20, #23), music (#22), pacing (#24), mission end + campaign map (#25). Continue the same loop (`scripts/runtest.ps1 debug`, the owner plays after saying "start"; stack in `build\runtest\cdb.log`).
1. **Sound effects (next task).** Owner: "almost all sound effects are missing"; music and the sidebar click work. Method: (a) temporary logging in `Play_Sample`/`Sample_Copy`/`Play_Sample_Handle` (what is requested, what comes back, `scomp` type, `DestPtr`, volume) and an environment-gated self-test that plays a few known effects (`MFCD::Retrieve("...AUD")`) right after `Init_Game` - this lets you check without the owner; (b) check `SCOMP_WESTWOOD` (`WIN32LIB/AUDIO/AUDUNCMP.CPP`, never executed before the sidebar click) against `AUDUNCMP.ASM`; (c) look for 32-bit truncations / stubs in the mixer path (`SOUNDINT.CPP` interrupt/timer callback, `SOUNDIO.CPP`, `Sound_Callback`, sample limits `MAX_SFX`, the `StreamLowImpact` handling); (d) compare what the original does when `Options` volume is applied. Verify by ear with the owner only after the self-test is satisfied.
2. **Movies** - decide with the owner ([movies.md](movies.md)); nothing to do until then.
3. Play on: mission 2+, options menu, save/load, in-game dialogs; each crash -> root cause -> pitfalls doc.
4. Run the remaining proactive audits ([../Analysis/summary.md](../Analysis/summary.md)): WIN32LIB on-disk structs vs 2-byte packing, enum audit leftovers, save-game struct layout.
5. Method reminders: dump the game's own buffers and compare with an independent Python decode for visual bugs (#15); environment-gated startup self-test for anything deep in the game ([../HANDOFF.md](../HANDOFF.md) section 5).
