# Modules — Index

Per top-level-directory documentation. See [../overview.md](../overview.md) for the whole-repo picture first.

- **[code/summary.md](code/summary.md)** — `CODE/` (589 files), the main game engine. Split into 7 sub-documents by subsystem (see that index):
  - [object_hierarchy.md](code/object_hierarchy.md) — base classes and concrete unit/building/object types
  - [map_display_ui.md](code/map_display_ui.md) — map/cell model, rendering pipeline, UI widget toolkit, dialogs
  - [ai_scripting_scenario.md](code/ai_scripting_scenario.md) — team/trigger scripting, scenario/rules data, superweapons
  - [networking_multiplayer.md](code/networking_multiplayer.md) — event/lockstep system, connections, IPX/TCP/WOL transports
  - [file_io_compression_saveload.md](code/file_io_compression_saveload.md) — Straw & Pipe filter framework, file abstraction, `.MIX`, save/load
  - [platform_audio_misc.md](code/platform_audio_misc.md) — audio, memory, input, CPU/OS detection, generic utilities
  - [build_entrypoint_eng_wolapi.md](code/build_entrypoint_eng_wolapi.md) — entry point/main loop, build system, `CODE/ENG` localization, `CODE/WOLAPI` COM bindings, backup-file notes
- **[win32lib.md](win32lib.md)** — `WIN32LIB/` (568 files), Westwood's Win32-native low-level support library — one section per subdirectory (AUDIO, DRAWBUFF, SHAPE, TILE, FONT, PALETTE, KEYBOARD, MEM, MOVIE, WINCOMM, etc).
- **[wwflat32.md](wwflat32.md)** — `WWFLAT32/` (400 files), the DOS-extender predecessor library WIN32LIB was ported from — one section per subdirectory, plus its relationship to WIN32LIB.
- **[vq_winvq.md](vq_winvq.md)** — `VQ/` + `WINVQ/` (399 files combined), the VQA video codec: core algorithm, container format, engine-integrated library vs. SDK/tools packaging.
- **[ipx_launch_launcher_tools.md](ipx_launch_launcher_tools.md)** — `IPX/`, `LAUNCH/`, `LAUNCHER/`, `TOOLS/` (126 files combined): LAN networking thunk DLLs, the DOS pre-launch stub, the Win32 patch launcher, and developer asset tools.
