---
name: codebase-history-and-cruft
description: The repo's layered history — coexisting subsystem generations, DOS-to-Win32 library ports, and where dead/backup code lives
metadata:
  type: knowledge_base
---

# Codebase History & Cruft

This codebase accreted across ~1994-1998 (and includes at least one file vendored from a *different* Westwood title). Understanding the layering prevents mistaking historical/dead code for the active implementation.

## The DOS → Win32 library port: WWFLAT32 vs WIN32LIB
[WWFLAT32/](../project_state/modules/wwflat32.md) is the earlier, DOS-extender-era ("DOS4GW") generation of Westwood's support library — real/protected-mode segment:selector plumbing, a dedicated `DESCMGMT` selector-management module, real-mode interrupt-hooked keyboard/mouse (`KEYIREAL.ASM`/`KEYIPROT.ASM`), and separate MCGA (mode 13h linear) vs. SVGA/VESA (bank-switched) graphics backends (`MCGAPRIM`/`SVGAPRIM`). [WIN32LIB/](../project_state/modules/win32lib.md) is its ported, Win32-native evolution: plain C++ classes fed by Windows messages instead of interrupt hooks, DirectDraw instead of MCGA/VESA mode-setting, no selector management needed (flat Win32 memory). Many same-named subdirectories (AUDIO, DIPTHONG, FONT, IFF, KEYBOARD, MEM, MISC, MONO, PALETTE, PLAYCD, SHAPE, SRCDEBUG, TILE, TIMER, WSA) share near-identical algorithm files (some literally byte-identical, e.g. shared `DS_*.ASM` shape-drawing variants) while their platform-facing internals diverge. Both were maintained in parallel during Westwood's DOS→Windows transition; Red Alert (1996) shipped from both lineages (DOS build + early Windows build).

## Three generations of online multiplayer backend, coexisting in `CODE/`
1. **IPX LAN** (1996, original) — `CODE/IPX*.CPP/.H` (game-side protocol) calling into the top-level [IPX/](../project_state/modules/ipx_launch_launcher_tools.md) 16-bit thunk DLLs (`THIPX16.DLL`/`THIPX32.DLL`), needed because 32-bit Win95 code can't call the real-mode DOS IPX interrupt directly. Author's own comment in `IPX/IPX16A.ASM`: *"in Win95, DOS IPX services are stubbed to just pass up to NWLINK... if we ever have direct access to that then all this code can go away."*
2. **TEN (Total Entertainment Network)** — `CODE/TENMGR.CPP/.H`, an early online-play backend, superseded by both the TCP/IP path and WOLAPI below. Now effectively dead but not removed.
3. **TCP/IP + Winsock** (`CODE/TCPIP.CPP/.H`, `WSPROTO.*`, `WSPIPX.*`, `WSPUDP.*`) and **WOLAPI** (Westwood Online) — the newest layer, added 1998 (`WOLAPIOB.H`'s header comment: "ajw 07/10/98"), roughly a year after the base game's 1997 `$Header` dates. Conditionally compiled behind `#ifdef WOLAPI_INTEGRATION`; grafted onto the original netcode rather than replacing it. See [project_state/modules/code/networking_multiplayer.md](../project_state/modules/code/networking_multiplayer.md).

## Two related-but-distinct scripted-AI systems (no single "SCRIPT.CPP")
- **Team scripting**: `TEAM.CPP`/`TEAMTYPE.CPP` — runtime squads executing an authored mission script (`TeamMissionType`: attack, move, guard, loop, deploy, etc).
- **Trigger system**: `TRIGGER.CPP`/`TRIGTYPE.CPP`/`TACTION.CPP`/`TEVENT.CPP` — map-event-driven condition→action pairs.
A future agent looking for "the AI scripting system" should check both — see [project_state/modules/code/ai_scripting_scenario.md](../project_state/modules/code/ai_scripting_scenario.md).

## Multi-toolchain, multi-machine build history
`CODE/`'s build files alone span Watcom (current), Borland/Turbo C++ (legacy), and Codewright IDE project files (`.PJT`, pointing at `C:\projects\redalert\code\`) for at least two machines/build targets ("home" vs. "work", per `RA-HDOS.PJT`/`RA-HOME.PJT`). `CODE/PROFILE.DEF` (a Watcom profiler config) references object-file module names (`CACHE.OBJ`, `CURSOR.OBJ`, `MEMORY.OBJ`, `TEXT.OBJ`, `BUTTON.OBJ`, `EXTFILE.OBJ`, `MONO.OBJ`) that **no longer exist** as separate `.CPP` files in this snapshot — evidence of later renames/merges (e.g. `MONO`→`MONOC.CPP`, `BUTTON`→folded into the `GADGET`/`*BTN` widget family). See [build_toolchains.md](build_toolchains.md).

## A tool vendored from a different Westwood game
`WINVQ/VQAVIEW`'s own `MAKEFILE` header banner reads *"Project Name: Blade Runner"* (Mike Grayford, Nov 1995) and its build depends on a `%BLADE%` environment variable — strong evidence this Win32 GUI VQA viewer tool was built for Westwood's *Blade Runner* (1997) and reused/vendored into the Red Alert tree rather than authored for Red Alert. See [../project_state/modules/vq_winvq.md](../project_state/modules/vq_winvq.md).

## Where dead/backup code concentrates (don't mistake for active source)
- **`.BAK` files**: dozens scattered across nearly every module (see [../agent_policies/toolchain_and_editing_constraints.md](../agent_policies/toolchain_and_editing_constraints.md)). Notably, `CODE/CONQUER.CPP.BAK` differs from the live `CONQUER.CPP` by ~1954 lines and `CODE/INIT.CPP.BAK` by ~1229 lines — real historical snapshots of substantial rewrites, worth consulting if tracing how the main loop/init sequence evolved. Others (e.g. `WWFLAT32/KEYBOARD/KEYIREAL.IBN`-adjacent `.BAK`s) are near-identical no-op leftovers.
- **Superseded module-copy subdirectories**: `WIN32LIB/{AUDIO,KEYBOARD,WW_WIN}/OLD/`, `WIN32LIB/MEM/{OLDMEM,MSVC}/`, `WWFLAT32/{AUDIO,KEYBOARD}/NEW/` and `KEYBOARD/X/`, `WWFLAT32/MEM/OLD/`, `WINVQ/VQA32/OLD/`. Exception: `IPX/OK/` is a **verified-good reference snapshot** (name = "OK", last known-working state), not dead code, while `IPX/OLD/` (singular file `IPX16A.ASM`) genuinely is superseded.
- **`WIN32LIB/SRCDEBUG/` and `WWFLAT32/SRCDEBUG/`**: flattened, no-makefile directories aggregating source from many other modules — almost certainly a debugger "source search path" snapshot rather than an active module. `WIN32LIB/SRCDEBUG/` is the only surviving location for three otherwise-vanished modules: a FILE-cache module, a VESA SVGA-mode module, and a VESA/VGA "Video Buffer" module — worth checking there if a reference to one of those turns up elsewhere with no owning directory.
