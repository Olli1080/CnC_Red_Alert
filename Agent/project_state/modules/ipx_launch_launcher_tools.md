---
name: ipx-launch-launcher-tools
description: IPX LAN thunk DLLs, the DOS pre-launch stub, the Win32 patch launcher, and developer asset tools
metadata:
  type: project_state
---

# IPX/ + LAUNCH/ + LAUNCHER/ + TOOLS/

Parent index: [summary.md](summary.md). All four are small, largely independent top-level directories.

## IPX/ — Novell IPX/SPX networking layer (52 files, subdirs `OK/`, `OLD/`)
Implements LAN multiplayer transport over Novell IPX/SPX. Because 32-bit Win95 apps can't call the real-mode DOS IPX interrupt directly, this module builds **16-bit "thunk" DLLs** (`THIPX16.DLL`, `THIPX32.DLL`) bridging 32-bit game code down to 16-bit code, which calls the DOS IPX vector via DPMI. Built with Watcom (`wpp`/`wlink`/`wlib`) + Microsoft's `THUNK.EXE` thunk compiler + TASM.

**This is distinct from `CODE/IPX*.CPP`** (the game-side protocol/connection logic — see [code/networking_multiplayer.md](code/networking_multiplayer.md)); `IPX/` here builds only the DLLs that bridge to the real-mode interrupt.

Captures multiple generations of the same thunking mechanism: `WWIPX*`/`OLTHIPX.ASM` (earlier/simpler thunk pair, Jan 16 1996) superseded by `THIPX*`/`OLTHIPX.ASM` (current at ship time, Jan 17 1996) but retained. Key files: `IPX16.H` (only exported symbol: `IPX_Initialise`), `IPX16C.CPP` (16-bit DLL glue), `IPX16A.ASM`/`.BAK` (real DOS-IPX-interrupt calling code, receive-buffer management, by Steve Tall Jan 1996 — his own comment: *"in Win95, DOS IPX services are stubbed to just pass up to NWLINK... if we ever have direct access to that then all this code can go away"*), `FIXTHUNK.CPP` (post-processes `THUNK.EXE` output), `THIPX.THK` (thunk-compiler definition declaring the actual API surface: `IPX_Initialise`, `IPX_Open_Socket95`, `IPX_Close_Socket95`, `IPX_Get_Connection_Number95`, `IPX_Send_Packet95`, `IPX_Broadcast_Packet95`, `IPX_Get_Local_Target95`, `IPX_Start_Listening95`, `IPX_Shut_Down95`, `IPX_Get_Outstanding_Buffer95`), `THIPX.ASM`/`THIPX16C.CPP`/`THIPX32C.CPP`/`THMAP16.ASM`/`THMAP32.ASM` (generated thunk glue), `PCMACRO.16` (shared asm macros, also in `LAUNCH/`), `MAKEFILE`/`MAKE.BAT`/`MAKETH16.BAT`/`MAKETH32.BAT`.

`IPX/OK/` = a **verified-good reference snapshot** ("OK" = last known-working state) of the whole build tree, near-duplicate of top level. `IPX/OLD/` = a genuinely superseded earlier draft of `IPX16A.ASM` only. Several `.BAK` files are stray build backups.

## LAUNCH/ — minimal DOS pre-launch stub (2 files)
The actual top-level executable a player runs — a tiny 16-bit real-mode stub (MASM 6). Job (per its own function-header block): check ≥400KB free conventional memory and ≥15MB free disk space (friendly error + keypress wait if not); `chdir` to its own directory; delete leftover `*.SWP` files; inject a `DOS4GVM=SwapMin:12M,SwapInc:0` env var (config for the DOS/4GW Professional extender); `DOS_LOAD_AND_EXEC` (INT 21h AH=4Bh) the real game binary, hardcoded as **`GAME.DAT`**; free the temp environment block and exit with the child's return code. By Steve Tall, Aug 1995 (original C&C), "Modified for RA, October 14th 1996." **Distinct from `LAUNCHER/` below** — this is a DOS memory/disk-space gate and DOS4GW env setup shim, not the Win32 patch-and-run launcher. Files: `LAUNCH.ASM` (the whole stub), `PCMACRO.16` (shared macro include, same as `IPX/`'s copy). No makefile — build via a source-comment instruction.

## LAUNCHER/ — Win32 patch-and-run launcher (47 files, subdir `UTIL/`)
A separate, later (Feb 1998, Neal Kettler) **Win32 GUI app** built with **MSVC 6** (`LAUNCHER.DSP`/`.DSW` — a clean break from Watcom/Borland used elsewhere). The "online launcher": reads a config file, checks a "Patches" folder, applies pending patches, launches the real game (or another configured app), waits for exit, loops to check for new patches again. Its own header documents it as generic, reused infrastructure "for games/apps using the chat API" from Westwood Online/"Carpenter" (the RA ladder-creator project) — not RA-specific.

**Control flow**: `WINBLOWS.CPP:WinMain` (hand-rolled command-line→argv tokenizer) → `MAIN.CPP:main` — `InitCommonControls`, optional debug log to `launcher.out`, `chdir` to install dir, load `.lcf` config via `ConfigFile::readFile`, `Read_Process_Info`, loop `Find_Patch`→`Apply_Patch` until none left, `Delete_Patches`, `Create_Process`+`Wait_Process` to run the game, re-check for patches after it exits. Config format: `#`-comments, `RUN = <dir> <exe>`, `SKU<n> = <id> <registry key>`.

Key files: `MAIN.CPP`, `WINBLOWS.CPP`/`.H`, `DIALOG.CPP`/`.H` (patch-progress dialog), `PATCH.CPP`/`.H` (`Apply_Patch` dispatch by type), `FINDPATCH.CPP`/`.H` (recognizes 4 patch types by extension: `.web` = link to a page to manually download, `.exe` = installer requiring reboot via `RunOnce` registry key, `.exn` = executable patch run immediately, `.rtp` = RTPatch binary-diff applied now), `CONFIGFILE.CPP`/`.H` (`ConfigFile` INI-like parser — header says "Project: Carpenter"), `PROCESS.CPP`/`.H` (`CreateProcess`/`WaitForSingleObject` wrapper), `DICTIONARY.H` (generic hash-table template), `WSTRING.CPP`/`.H` (custom string class predating portable `std::string` use here), `WDEBUG.CPP`/`.H`+`MONOD.CPP`/`.H`+`STREAMER.CPP`/`.H`+`ODEVICE.H`+`FILED.H` (pluggable debug-logging framework: `OutputDevice` base, `Streamer` routes `DBGMSG(...)`-style output), `LOADBMP.CPP`/`.H` (splash-screen bitmap loader, actually used), `LAUNCHER1.RC`/`RESOURCE.H`/`REDALERT.ICO` (string table + patch-progress/splash dialogs), `LAUNCHER.DSP`/`.DSW` (MSVC6 project, links `comctl32.lib`, `/subsystem:windows`).

**Dead/unreferenced** (on disk, not in `LAUNCHER.DSP`'s `SOURCE=` list): `BITMAP.CPP`, `256BMP.C` — an older DIB-loading implementation superseded by `LOADBMP.CPP`.

`LAUNCHER/UTIL/` — a parallel copy of the debug-output framework plus extra unused `OutputDevice` implementations (`SYSLOGD.CPP`/`.H` wraps Unix `syslog()` — dead on Windows, evidence of shared cross-platform library origin; `MBOXD.H`, `STDERRD.H`/`STDOUTD.H`). Diffing shows `UTIL/WDEBUG.CPP` is the **newer, thread-safe** version (adds a `Sem4 DebugLibSemaphore`) vs. the top-level copy actually compiled into `launcher.exe` — the canonical/updated shared library source left un-synced with what's actually built.

## TOOLS/ — developer/build-pipeline tools (25 files, subdirs `AUDIOMAK/`, `MIX/`)
Most tools ship as **prebuilt DOS `.EXE` binaries only, no source in this repo**: `ANIMATE.EXE` (+ superseded `ANIMATE.EXE.OLD`, 129,988 vs 141,664 bytes), `FILEDATA.EXE`, `FONTMAKE.EXE`, `GETREG.EXE`, `ICONCOMP.EXE`, `ICONMAP.EXE`, `KEYFRAME.EXE`, `MAP2MAP.EXE`, `WWCOMP.EXE`, `WWPACK.EXE` — see [../../Analysis/summary.md](../../Analysis/summary.md) for the open question of whether matching source exists elsewhere. Only `AUDIOMAK/` and `MIX/` ship full source (each also has a top-level convenience binary copy).

### TOOLS/AUDIOMAK/ — `.AUD` audio asset compiler
`AUDIOMAK.CPP` (Joe L. Bostic, Aug 1994, "AUDIOMAK V2.3"): `AUDIOMAK [option] <sourcefile> <destfile>`, options `-X-` (disable compression), `-V` (verbose stats). Reads RIFF/WAVE or VOC input; writes the `.AUD` header + compressed data (see [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md) for the format). `SCODE.ASM` — the actual Westwood delta-compression codec. Borland toolchain (`MAKEFILE` generates `turboc.cfg`/`tasm.cfg`/`tlink.cfg`; `TURBOC.CFG`/`TASM.CFG`/`TLINK.CFG` show Borland C++ 4.5 + TASM, large model, hardcoded to drive `D:`).

### TOOLS/MIX/ — `.MIX` archive packer
`MIXFILE.CPP` (Joe L. Bostic, Aug–Oct 1994, "MIXFILE V1.4"): `DATFILE <controlfile> <datafile>` with options `-q` (quiet), `-i<path>` (alt search path, up to 10), `-e<ext1>=<ext2>` (rename extensions on pack, up to 10), `-s` (record full paths in the CRC ID). Control file = plain-text filename list (`fscanf %s`), **not** an `.INI` file. `DataClass` implements the confirmed on-disk `.MIX` layout documented in [../../knowledge_base/file_formats.md](../../knowledge_base/file_formats.md). `MIXFILE.IDE` — binary Borland C++ 4.5 IDE project (no separate makefile; build driven entirely by the IDE project).

No top-level `TOOLS/` makefile ties the tools together — each subdirectory is self-contained.
