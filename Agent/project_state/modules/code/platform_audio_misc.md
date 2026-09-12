---
name: code-platform-audio-misc
description: CODE/ audio playback, memory/heap, keyboard/mouse input, CPU/OS detection, and general utility headers
metadata:
  type: project_state
---

# CODE/ — Audio, Platform & Misc Utilities

Parent index: [summary.md](summary.md).

## Audio
- `AUDIO.CPP`/`.H` (+ stale `.CPP.BAK`) — `AudioClass`/sound-effect playback engine; wraps MCI/DirectSound-style playback with volume management.
- `ADPCM.CPP` — ADPCM audio decompression codec used for `.AUD`/`.VOC` sound assets (see [../../../knowledge_base/file_formats.md](../../../knowledge_base/file_formats.md)).
- `THEME.CPP`/`.H` — background music (score) track selection/playback controller.

## Memory / low-level data structures
- `ALLOC.CPP`, `HEAP.CPP`/`.H`, `MEMCHECK.H` — custom heap/memory allocator and leak-checking.
- `DYNAVEC.CPP`, `VECTOR.CPP`/`.H` — `DynamicVectorClass<T>` generic growable-array template, used widely (e.g. `LayerClass` in [map_display_ui.md](map_display_ui.md)).
- `LINK.CPP`/`.H`, `LISTNODE.H` — intrusive linked-list base classes.
- `RECT.CPP`/`.H`, `REGION.H`, `FIXED.CPP`/`.H` — geometry rectangle helper and fixed-point number type (`fixed`, used pervasively for rules biases).
- `RANDOM.CPP`/`.H`, `RAND.CPP`, `RNG.H`, `RNDSTRAW.*` — deterministic PRNG (critical for lockstep multiplayer sync, see [networking_multiplayer.md](networking_multiplayer.md)) and a straw-filter wrapper for random data.

## Input
- `KEY.CPP`/`.H`, `KEYBOARD.CPP`/`.H` — keyboard input handling.
- `MOUSE.CPP`/`.H` — mouse input/cursor handling.

## Debug / platform / CPU
- `DEBUG.CPP`/`.H` — debug/assert utilities (string tables in `CODE/ENG/DEBUG.TXT`, see [build_entrypoint_eng_wolapi.md](build_entrypoint_eng_wolapi.md)).
- `DPMI.CPP`/`.H` — DOS Protected Mode Interface glue (real/protected mode transitions) for the DOS build.
- `GETCPU.CPP`, `CPUID.ASM` — CPU feature detection.
- `BENCH.CPP`/`.H` — performance benchmarking harness.
- `VERSION.CPP`/`.H`, `VER_FUNC.TXT` — build/version string stamping.
- `W95TRACE.CPP`/`.H` — Windows 95 debug trace output helper.
- `WINASM.ASM`, `SUPPORT.ASM`, `2SUPPORT.ASM`, `2TXTPRNT.ASM`, `TXTPRNT.ASM` — misc ASM support routines (text printing, low-level Windows glue).

## Umbrella headers (included nearly everywhere)
- `GLOBALS.CPP` — central definition point for the game's many global variables (houses array, object heaps, etc).
- `DEFINES.H`, `EXTERNS.H`, `FUNCTION.H`/`FUNCTION.I`, `TYPE.H`, `COMPAT.H`, `WATCOM.H`, `INLINE.H`, `LANGUAGE.H`, `STYLE.H` — `FUNCTION.H` in particular pulls in most subsystem headers and is effectively the project's precompiled-header equivalent (included by `WINSTUB.CPP`, `CONQUER.CPP`, etc).

## Numeric / stub glue
- `NUMBER.CPP`, `EXPAND.CPP` — numeric formatting / macro-expansion helpers.
- `CLASS.CPP`, `STUB.CPP`, `CWSTUB.C` — miscellaneous stub/glue translation units.
