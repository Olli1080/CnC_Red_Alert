# Agent Knowledge Base — Index

This folder is the persistent knowledge base for AI agents working in this repository (EA's open-source release of Command & Conquer: Red Alert, 1996 Westwood Studios). See the root [AGENTS.md](../AGENTS.md) for the one-paragraph orientation.

## Start here when resuming the port
- **[HANDOFF.md](HANDOFF.md)** — takeover document: current state, the exact next blocker, the build/run/debug workflow, rules and pitfalls. Then [plans_wip/boot_to_main_menu.md](plans_wip/boot_to_main_menu.md).

## Subfolders

- **[agent_policies/](agent_policies/summary.md)** — Rules to respect: licensing/attribution constraints, contribution scope (this is a personal fork; upstream takes no PRs), and toolchain/editing caveats for this legacy codebase.
- **[project_state/](project_state/summary.md)** — The structural map of the codebase itself: what every top-level module (`CODE/`, `WIN32LIB/`, `WWFLAT32/`, `VQ/`+`WINVQ/`, `IPX/`+`LAUNCH/`+`LAUNCHER/`+`TOOLS/`) does, organized down to key-file-level descriptions.
- **[knowledge_base/](knowledge_base/summary.md)** — Emerging, cross-cutting knowledge: the build/toolchain landscape, missing third-party dependencies, the repo's proprietary file formats, its accumulated history/cruft (backup files, superseded module copies, multi-generation subsystems), and the modernization/porting knowledge (debugging & runtime-testing runbook, x64 pitfalls checklist, the `/zp1` struct-packing saga).
- **[Analysis/](Analysis/summary.md)** — Candidate and in-progress deep-dive investigation tasks for future sessions (e.g. tracing the full game loop, mapping the save-game format, documenting `.MIX`/`.INI` schemas in full).
- **[plans_wip/](plans_wip/summary.md)** — Work plans currently in progress.
- **[plans_done/](plans_done/summary.md)** — Completed work plans, kept for reference.

## How this knowledge base was built

Initial population (2026-09-12): a full pass over the repository using parallel exploration agents, one per module group, each opening representative source files rather than guessing from filenames. See [project_state/summary.md](project_state/summary.md) for what was covered and what wasn't (some of the ~2,900 files are grouped by obvious naming pattern rather than individually described — flagged per-file in [Analysis/summary.md](Analysis/summary.md) where deeper individual coverage would help).

## Maintenance rule

When you learn something non-obvious about this codebase — an architectural relationship, a gotcha, a build fix, a decision and its rationale — add it here rather than leaving it only in chat history. Keep leaf files under ~400 lines; split by orthogonal concern (not just size) into a subfolder with its own `summary.md` when a topic grows past that. Update the relevant `summary.md` index whenever you add, move, or remove a file.
