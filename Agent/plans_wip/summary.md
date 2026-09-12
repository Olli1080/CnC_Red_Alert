# Plans (In Progress) — Index

- **[modernization_cmake_vcpkg.md](modernization_cmake_vcpkg.md)** — Rebuild the build system on CMake + vcpkg (as a git submodule), targeting current MSVC (VS2026) and modern Windows APIs, replacing the legacy Watcom/Borland/MSVC6 toolchain sprawl. Status: Phase 0 complete; sub-milestones 1a and 1b complete (see [plans_done](../plans_done/summary.md)); sub-milestone 1c (wire in `.ASM`, attempt a real executable link) is next.

When starting other non-trivial work (e.g. an item in [../Analysis/summary.md](../Analysis/summary.md), or a user-requested feature/fix), add a short-lived plan file here describing the approach and open questions, link it from this index, and move it to [../plans_done/summary.md](../plans_done/summary.md) when finished.
