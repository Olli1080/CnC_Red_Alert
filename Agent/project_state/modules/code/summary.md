# CODE/ — Index

`CODE/` (589 files: ~570 directly in the folder, plus `CODE/ENG/` and `CODE/WOLAPI/`) is the entire game-engine source for Red Alert's DOS/Win32 executable. Watcom C++ with hand-written x86 assembly (TASM) for hot paths, targeting both a DOS build and a Win32 build from the same tree (see [build_entrypoint_eng_wolapi.md](build_entrypoint_eng_wolapi.md) for how `MAKEFILE` selects platform/language).

Architecturally: a deep single-inheritance class hierarchy for all battlefield objects (`AbstractClass → ObjectClass → MissionClass → RadioClass → TechnoClass → FootClass → {UnitClass, InfantryClass, AircraftClass, VesselClass}`, with `BuildingClass` branching off `TechnoClass` directly) — see [object_hierarchy.md](object_hierarchy.md) — topped by a `GScreenClass → MapClass → DisplayClass → SidebarClass/RadarClass` UI/rendering stack — see [map_display_ui.md](map_display_ui.md) — driven by scripted team/trigger logic ([ai_scripting_scenario.md](ai_scripting_scenario.md)), synchronized across the network via a lockstep event queue ([networking_multiplayer.md](networking_multiplayer.md)), and persisted through a composable stream-filter I/O framework ([file_io_compression_saveload.md](file_io_compression_saveload.md)).

## Sub-documents

| File | Covers |
|---|---|
| [object_hierarchy.md](object_hierarchy.md) | Core base classes (`AbstractClass`…`FootClass`, mixins) and concrete object/unit types (`UnitClass`, `BuildingClass`, `BulletClass`, `AnimClass`, etc.), the `*DATA.CPP` type-class family, house/player logic (`HouseClass`) |
| [map_display_ui.md](map_display_ui.md) | Map/cell/coordinate model, pathfinding, the built-in map editor, the `GScreenClass`-rooted rendering pipeline, the retro GUI widget toolkit and dialog screens |
| [ai_scripting_scenario.md](ai_scripting_scenario.md) | `TeamClass`/`TriggerClass` scripted-AI systems, `ScenarioClass`/`RulesClass` data loading, superweapons |
| [networking_multiplayer.md](networking_multiplayer.md) | `EventClass`/`QueueClass` lockstep determinism, `ConnectionClass` hierarchy, IPX/null-modem/TCP-IP/WOLAPI transports, `SessionClass` |
| [file_io_compression_saveload.md](file_io_compression_saveload.md) | `Pipe`/`Straw` filter framework (LCW/LZO/LZW/Blowfish/SHA/CRC/PK/Base64), file-class hierarchy, `.MIX` reader, `.INI`/rules parsing, save/load |
| [platform_audio_misc.md](platform_audio_misc.md) | Audio playback (`AudioClass`, ADPCM), memory/heap, keyboard/mouse input, CPU/OS detection, geometry/fixed-point/RNG utilities, umbrella headers |
| [build_entrypoint_eng_wolapi.md](build_entrypoint_eng_wolapi.md) | Entry point (`STARTUP.CPP`→`Main_Game()`→`Main_Loop()`), build system files, `CODE/ENG/` localization tables, `CODE/WOLAPI/` COM bindings + root-level `WOL*`/`WOLAPIOB`/`RAWOLAPI` glue, `.BAK`/oddity notes |

## Cross-references
- Two of these subsystems are further contextualized in [../../../knowledge_base/codebase_history_and_cruft.md](../../../knowledge_base/codebase_history_and_cruft.md): the three coexisting online-multiplayer generations (IPX → TEN → TCP/IP+WOLAPI), and the "no single SCRIPT.CPP" team-vs-trigger scripting split.
- File formats defined here (`.MIX`, `.INI`) are cataloged in [../../../knowledge_base/file_formats.md](../../../knowledge_base/file_formats.md).
