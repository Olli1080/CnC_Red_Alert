# AGENTS.md

This repository is Electronic Arts' 2025 open-source release of the **Command & Conquer: Red Alert** (1996, Westwood Studios) source code — a ~2,900-file, mid-1990s Watcom C++ / Borland TASM codebase spanning a DOS engine generation and its Win32 port. The original Watcom/Borland toolchain is unobtainable (see [Agent/knowledge_base/build_toolchains.md](Agent/knowledge_base/build_toolchains.md)), but **a modernized CMake + vcpkg build (MSVC 2026, x64) now produces a linking `redalert.exe`** that boots against the owner's own game data and is **playable from the main menu through the first mission, the score screen and the campaign map** (rendering via a Direct3D 11 presenter, mouse input, music; verified by the owner). Still open: most sound effects are missing, movies are skipped, and mission 2 onwards is unverified.

**Continuing the modernization work? Start with [Agent/HANDOFF.md](Agent/HANDOFF.md)** — current state, the exact next blocker, the build/run/debug workflow (`scripts/runtest.ps1`), and rules to keep.



**Before doing any other non-trivial work here, read [Agent/summary.md](Agent/summary.md).** It indexes everything this project's agents have already learned:

- [Agent/HANDOFF.md](Agent/HANDOFF.md) — the takeover document (read first when resuming the port).
- [Agent/agent_policies/](Agent/agent_policies/summary.md) — licensing, contribution, and toolchain constraints you must respect.
- [Agent/project_state/](Agent/project_state/summary.md) — map of every module (CODE, WIN32LIB, WWFLAT32, VQ/WINVQ, IPX/LAUNCH/LAUNCHER/TOOLS) with key files described.
- [Agent/knowledge_base/](Agent/knowledge_base/summary.md) — build system, dependencies, file formats, repo history/cruft, and the porting knowledge (debugging runbook, x64/packing pitfalls).
- [Agent/Analysis/](Agent/Analysis/summary.md) — candidate deep-dive investigations not yet done.
- [Agent/plans_wip/](Agent/plans_wip/summary.md) / [Agent/plans_done/](Agent/plans_done/summary.md) — in-progress and completed work plans.

This repo's own remote is a personal fork (`origin` = `Olli1080/CnC_Red_Alert`) of EA's upstream (`upstream` = `electronicarts/CnC_Red_Alert`). EA's upstream repository does not accept pull requests or issues (see the root [README.md](README.md)) — it exists for Steam Workshop mod support and preservation.

Keep this knowledge base current: when you learn something non-obvious about the codebase, add or update a file under `Agent/` (and its folder's `summary.md`) rather than letting the knowledge live only in a chat transcript.
