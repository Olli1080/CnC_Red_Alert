# Analysis — Index

Candidate and in-progress deep-dive investigation tasks for future sessions. Nothing here has been executed yet unless noted; this is a backlog, not a report.

## Candidate investigations (not yet started)

- **Full game-loop trace.** [project_state/modules/code/build_entrypoint_eng_wolapi.md](../project_state/modules/code/build_entrypoint_eng_wolapi.md) identifies the call chain `STARTUP.CPP:WinMain/main` → `CONQUER.CPP:Main_Game()` → `Main_Loop()`, but nobody has traced what happens inside `Main_Loop()` frame-by-frame (order of input processing, `LogicClass` AI ticks, rendering, event queue flush). Worth a dedicated pass if working on gameplay-timing bugs or a reimplementation.
- **GCL (Greenleaf Communications Library) usage.** [knowledge_base/dependencies_and_missing_libs.md](../knowledge_base/dependencies_and_missing_libs.md) flags that the README lists GCL as a dependency, but `WIN32LIB/WINCOMM/` explicitly says it *replaces* GCL — so something else in the tree (likely a DOS-era path, possibly in `WWFLAT32/` or an older part of `CODE/`) must still call it. Not yet located.
- **Save-game format.** `CODE/SAVELOAD.CPP` implements `Save_Game`/`Load_Game`/`Code_All_Pointers`/`Decode_All_Pointers`, but the on-disk layout hasn't been documented the way `.MIX`/`.AUD`/`.VQA` have in [knowledge_base/file_formats.md](../knowledge_base/file_formats.md). Useful for save-editor tooling.
- **Full `.INI` schema for `RULES.INI` and scenario files.** `CODE/RULES.CPP` and `CODE/CCINI.CPP` parse these, but no one has enumerated the actual key/section vocabulary (unit stats, house names, trigger syntax) — needed for anyone hand-authoring or tooling scenario/mod content.
- **CODE/ map editor mode.** `MAPEDIT.CPP/.H`, `MAPEDDLG.CPP`, `MAPEDPLC.CPP`, `MAPEDSEL.CPP`, `MAPEDTM.CPP` were identified but not read in depth — how scenario authoring actually worked in-engine, relevant to anyone building modern mod tools.
- **The 8 `TOOLS/` binaries shipped without source** (`ANIMATE.EXE`, `FILEDATA.EXE`, `FONTMAKE.EXE`, `GETREG.EXE`, `ICONCOMP.EXE`, `ICONMAP.EXE`, `KEYFRAME.EXE`, `MAP2MAP.EXE`, `WWCOMP.EXE`, `WWPACK.EXE`) — confirm whether any matching source exists elsewhere in the tree (e.g. under a differently-named directory) before assuming they're permanently source-less.
- **VQ/WINVQ still-image format drift.** [project_state/modules/vq_winvq.md](../project_state/modules/vq_winvq.md) notes `VQ/INCLUDE/VQ.H`'s `_VQHeader` and `WINVQ/INCLUDE/VQFILE.H`'s `VQHeader` define non-identical field layouts for what looks like the same still-image VQ format — worth resolving which (if either) is actually used by shipped assets.
- **CODE/WOLAPI COM interface surface.** `CODE/WOLAPI/WOLAPI.H` is MIDL-generated (117KB) and was not read in full — only skimmed for its existence and role. A full pass would matter if reviving WOL-integration (`WOLAPI_INTEGRATION`) functionality.

## How to use this file
When starting one of these, move a short plan into [../plans_wip/summary.md](../plans_wip/summary.md) (with its own file if non-trivial), and once done, write the findings into the relevant `project_state/` or `knowledge_base/` file, then move the plan entry to [../plans_done/summary.md](../plans_done/summary.md) and remove/check off the item here.
