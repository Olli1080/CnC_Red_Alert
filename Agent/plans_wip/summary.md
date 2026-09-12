# Plans (In Progress) — Index

- **[modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md)** — Rebuild the build system on CMake + vcpkg (as a git submodule), targeting current MSVC (VS2026) and modern Windows APIs, replacing the legacy Watcom/Borland/MSVC6 toolchain sprawl. Status: Phase 0 complete and verified; Phase 1 broken into sub-milestones, 1a (WIN32LIB compiling standalone) actively in progress.
- **[win32lib_1a_port_log.md](win32lib_1a_port_log.md)** — Live progress log for sub-milestone 1a: 1650 build errors down to 173 so far, with the high-leverage root-cause fix (a `WWSTD.H` include-guard bug) documented along with every remaining file's diagnosis/hypothesis.

When starting other non-trivial work (e.g. an item in [../Analysis/summary.md](../Analysis/summary.md), or a user-requested feature/fix), add a short-lived plan file here describing the approach and open questions, link it from this index, and move it to [../plans_done/summary.md](../plans_done/summary.md) when finished.
