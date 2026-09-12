---
name: code-build-entrypoint-eng-wolapi
description: CODE/ entry point and main loop, build system files, ENG localization tables, WOLAPI COM bindings, backup-file notes
metadata:
  type: project_state
---

# CODE/ — Entry Point, Build System, ENG/WOLAPI Subfolders, Oddities

Parent index: [summary.md](summary.md). Build-toolchain cross-reference: [../../../knowledge_base/build_toolchains.md](../../../knowledge_base/build_toolchains.md).

## Entry point / main game loop
Canonical call chain to trace for frame-by-frame engine flow (not yet traced further — see [../../../Analysis/summary.md](../../../Analysis/summary.md)):
1. **`STARTUP.CPP`** — the true OS entry point: `int PASCAL WinMain(...)` (Win32 build) or `int main(int argc, char *argv[])` (non-Win32 build). Performs low-level engine/window init, then calls `Main_Game(argc, argv)`.
2. **`CONQUER.CPP`** —
   - `Main_Game(int argc, char *argv[])` (~line 215) — "the first official routine of the game": handles initialization and main game loop control.
   - `Main_Loop(void)` — the main game loop, called repeatedly from `Main_Game`.
   - `Map_Edit_Loop(void)` — parallel mini-loop used in map-editor mode instead of normal play.
   - `Message_Input(...)` — processes inter-player chat/message input during a session.
3. `WINSTUB.CPP` — Watcom/Win32 stub implementations for externals needed to link the Win32 build (`Create_Main_Window`, `Focus_Loss`, `Check_For_Focus_Loss`, `Memory_Error_Handler`).

## Resources
- `CC_ICON.RC`/`.RH`, `REDALERT.ICO`, `REDALERT_NO256.ICO` — Windows resource script and icon assets for the executable.

## Build system files
- **`MAKEFILE`** (Watcom `wmake`, 755 lines) — primary build script; `!ifdef WIN32`/`GERMAN`/`FRENCH` macros select platform and language, pointing at `..\watcom\binnt\wpp386`/`wlib`.
- **`MAKE_ALL.BAT`** — wraps `wmake` three times (`WIN32=1`, `WIN32=1 GERMAN=1`, `WIN32=1 FRENCH=1`) to build all three localized executables; sets `WATCOM` env var to `C:\rawolapi\SOURCE\WATCOM` (historical build-machine path).
- **`JOEMAKE.BAT`** — internal Westwood "network make" build-farm distribution script.
- **`RULES.MAK`, `BFILE.MAK`, `BFILE2.MAK`, `ADAMTEMP.MAK`** — large auxiliary `.MAK` dependency-list files (386–2122 lines), likely per-object-file compile dependencies for an older make setup (`ADAMTEMP` = a different engineer's personal/temp build variant).
- **`TASM.CFG`** — Turbo Assembler config (`-mx -ml`) for an older Borland toolchain path.
- **`TURBOC.CFG`, `TLINK.CFG`** — Borland Turbo C++/TLINK config referencing `bc402\include`, `d:\WATCOM\H`, `d:\library` — an even earlier Borland-based build predating the Watcom migration, retained as legacy artifact.
- **`.PJT` files** (`C&CZERO.PJT`, `RA-HDOS.PJT`, `RA-HOME.PJT`, `RA95.PJT`, `RADOS.PJT`) — Codewright text-editor project files (file-lists only, pointing at `C:\projects\redalert\code\`), one per build target/machine (DOS, Win95, home vs. work) — editor convenience, not part of compilation.
- **`REDALERT.IDE`, `CONQUER.IDE`** — binary Borland C++ IDE project files.
- **`PROFILE.DEF`** — Watcom sampling-profiler config listing object files/symbols to instrument (`PRFILE`, `NOPRSYMBOL` mangled Watcom vtable/exception-table symbols) — reveals the historical full object-file list, some of which (`CACHE`, `CURSOR`, `MEMORY`, `TEXT`, `BUTTON`, `EXTFILE`, `MONO`) no longer exist as separate `.CPP` files (see [../../../knowledge_base/codebase_history_and_cruft.md](../../../knowledge_base/codebase_history_and_cruft.md)).
- **Lint configs** — `STD.LNT`, `CONQUER.LNT`, `OPTIONS.LNT`, `CO-WC32.LNT` (PC-Lint static analysis, evidence the team ran a linter as part of QA).
- **`PROFILE.CPP`** — the actual in-game profiler instrumentation corresponding to `PROFILE.DEF`.

## `CODE/ENG/` — localization string tables
Not source code — plain-text tables of `# TXT_SYMBOL_NAME` markers followed by the English UI string, e.g. `# TXT_BUTTON_SELL` → `Sell` (from `CONQUER.TXT`). Top level: `CONQUER.TXT`/`.BAK`, `CREDITS.TXT`/`.BAK`, `DEBUG.TXT`/`.BAK`, `MESSAGE.TXT` (longer in-game advisor tooltip strings, e.g. `# MESSAGE_BUILD_WINDTRAP` → "You must build a Windtrap..."). Subfolders `ENG/`, `FRE/`, `GER/` each hold only `CONQUER.TXT`/`CREDITS.TXT` — the per-language variants the root `MAKEFILE`'s `LANGUAGE=ENGLISH|GERMAN|FRENCH` macro selects between. `.BAK` copies here are stale duplicates.

## `CODE/WOLAPI/` — Westwood Online COM bindings
MIDL-generated COM interface declarations (not hand-written): `WOLAPI.H` (117KB, "File created by MIDL compiler... Compiler settings for WOLAPI.idl") declaring interfaces like `IRTPatcher`, `IRTPatcherEvent`, `IChat`; `WOLAPI_I.C` (matching COM IID/CLSID definitions); `CHATDEFS.H`, `DOWNLOADDEFS.H`, `FTPDEFS.H`, `NETUTILDEFS.H` (supporting constants/structs for chat, patch-download, FTP, network-utility). This is the raw generated API surface — the hand-written glue that calls into it lives back in `CODE/` root as `WOLAPIOB.CPP`/`.H` and `RAWOLAPI.CPP`/`.H` (see [networking_multiplayer.md](networking_multiplayer.md)). Full interface surface not read in depth — see [../../../Analysis/summary.md](../../../Analysis/summary.md).

## Backup files / oddities
Seven `.BAK` files in `CODE/` root: `ANIM.CPP.BAK`, `AUDIO.CPP.BAK`, `CONNECT.CPP.BAK`, `CONQUER.CPP.BAK`, `INIT.CPP.BAK`, `KEYFBUFF.ASM.BAK`, `MSGBOX.CPP.BAK`. Their `$Header` VCS tags point at an older internal VCS (`F:\projects\c&c0\vcs\code\*.cpv`, Oct 1996) vs. the current files' tags (`/CounterStrike/*.CPP`, Mar 1997) — two successive internal project/branch names. Diff sizes vary hugely: `KEYFBUFF.ASM.BAK` is byte-identical to the live file (pure leftover); `CONNECT.CPP.BAK` differs by only 12 lines; but **`CONQUER.CPP.BAK` differs by ~1954 lines and `INIT.CPP.BAK` by ~1229 lines** — real historical snapshots of substantial rewrites to the main loop/init sequence, worth consulting for that history. `MSGBOX.CPP.BAK` differs by 74 lines, `ANIM.CPP.BAK` by 131, `AUDIO.CPP.BAK` by 264. `CODE/ENG/*.BAK` are similarly stale duplicate string tables. `REDALERT.IDE`/`CONQUER.IDE` are binary, not text — don't read/diff as text.
