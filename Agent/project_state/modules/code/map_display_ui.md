---
name: code-map-display-ui
description: CODE/ map/cell model, pathfinding, map editor, rendering pipeline, GUI widget toolkit and dialogs
metadata:
  type: project_state
---

# CODE/ — Map, Display & UI

Parent index: [summary.md](summary.md).

## Map / cell / world-state
- `CELL.CPP`/`.H` — `CellClass`: per-tile state (terrain template, overlay, occupying objects, radar-dirty flag).
- `MAP.CPP`/`.H` — `MapClass : GScreenClass`: the full cell grid; pathing helpers, region/zone queries, cell allocation.
- `COORD.CPP`, `COORDA.ASM` — coordinate math (cell↔pixel↔lepton conversions), with an ASM-optimized variant.
- `FACING.CPP`/`.H` — 8/32-direction facing math used by turret and movement code.
- `FINDPATH.CPP` — the unit pathfinding algorithm (line-of-sight + clockwise/counter-clockwise edge-following around obstacles).
- `MAPSEL.CPP` — scenario/map selection screen logic.
- `MAPEDIT.CPP`/`.H`, `MAPEDDLG.CPP`, `MAPEDPLC.CPP`, `MAPEDSEL.CPP`, `MAPEDTM.CPP` — the built-in scenario/map editor mode and its dialogs. Not yet read in depth — see [../../../Analysis/summary.md](../../../Analysis/summary.md).

## Rendering / display / UI screen stack
- `GSCREEN.CPP`/`.H` — `GScreenClass`: base "screen" class with `One_Time`/`Init`/`Init_IO` virtuals; root of the map+UI render pipeline.
- `DISPLAY.CPP`/`.H` — `DisplayClass : MapClass, LayerClass`: the tactical viewport — scrolling, cursor, cell pixel/icon size constants.
- `LAYER.CPP`/`.H` — `LayerClass : DynamicVectorClass<ObjectClass*>`: sorted draw-order layer of visible objects.
- `RADAR.CPP`/`.H` — minimap/radar rendering and interaction.
- `SIDEBAR.CPP`/`.H` — `SidebarClass`: the build-queue sidebar UI, ties into `FactoryClass`/`PowerClass`.
- `POWER.CPP`/`.H` — power-bar UI widget and power-balance bookkeeping tied to the sidebar.
- `SCROLL.CPP`/`.H` — edge/keyboard map scrolling.
- `SURFACE.CPP`/`.H` — low-level video-surface/blit wrapper.
- `SPRITE.CPP`, `KEYFRAME.CPP` (+ `2KEYFRAM.CPP`, `2KEYFBUF.ASM`) — shape/keyframe sprite drawing, with ASM-optimized frame-buffer variants.
- `ROTBMP.CPP`/`.H`, `BMP8.CPP`/`.H`, `DIBFILE.CPP`, `DIBUTIL.CPP`/`.H`, `WRITEPCX.CPP`, `FILEPCX.H` — bitmap rotation and PCX/DIB image loading/writing (mostly editor/tool support).
- `HSV.CPP`/`.H`, `RGB.CPP`/`.H` — color-space conversion for palette effects (fades, ion-cannon flash, etc).

## Retro GUI widget toolkit
A self-contained dialog-control system under a common `GadgetClass`:
- `GADGET.CPP`/`.H` (base), `CONTROL.CPP`/`.H`
- Controls: `SHAPEBTN`, `TEXTBTN`, `STATBTN`, `CHECKBOX`, `TOGGLE`, `SLIDER`, `GAUGE`, `EDIT`, `LIST`, `CHEKLIST`, `COLRLIST`, `TAB`, `TXTLABEL`, `TOOLTIP`
- `DIALOG.CPP` — modal dialog base.

## Dialog screens (`*DLG.CPP`)
`CONFDLG` (confirm), `DESCDLG` (scenario description), `GAMEDLG` (in-game options), `LOADDLG`/`SAVEDLG.H` (load/save), `MAPEDDLG` (map editor), `NETDLG` (network setup), `NULLDLG` (modem-null setup), `SEDITDLG` (scenario editor), `SOUNDDLG` (audio options), `VISUDLG` (visual/scenario options), `GOPTIONS.CPP`/`.H` / `OPTIONS.CPP`/`.H` (general options menu).

## Menus / sequences / help
- `MENUS.CPP` — main menu screen flow.
- `CREDITS.CPP`/`.H`, `INTRO.CPP`/`.H`, `ENDING.CPP`/`.H` — credits roll, intro movie sequencing, campaign endings.
- `MOVIE.H`, `MCI.CPP`/`.H`, `MCIMOVIE.CPP`/`.H` — VQA/MCI video playback for cutscenes (see [../vq_winvq.md](../vq_winvq.md) for the codec itself).
- `HELP.CPP`/`.H` — in-game contextual help bubbles.
- `MONOC.CPP`/`.H` — debug "mono" monitor output screen (secondary monochrome debug display), used by `Debug_Dump()` methods (`#ifdef CHEAT_KEYS`) throughout the codebase.
