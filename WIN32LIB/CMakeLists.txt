# WIN32LIB - Westwood's Win32 support library (see Agent/project_state/modules/win32lib.md).
#
# Sub-milestone 1a (Agent/plans_wip/modernization_cmake_vcpkg.md): get this compiling standalone
# under MSVC2026/x64/CMake. Only the "live" modules are built - SRCDEBUG/ (a flattened debug-source
# snapshot, not a real module) and every nested OLD/, OLDTEST/, TEST/, OLDMEM/, MSVC/, UTIL/
# subfolder (superseded duplicate copies) are deliberately excluded by using non-recursive globs
# scoped to each top-level module directory. EXAMPLE/ (a template app, not library code) is also
# excluded.
#
# .ASM files are not yet included (deferred until UASM is wired in) - this target only proves the
# C++ side compiles.

# MOVIE/ is excluded entirely: MOVIE.CPP requires the classic DirectShow SDK "baseclasses"
# headers (streams.h, amstream.h, ddstream.h, mmstream.h), which were never part of the
# redistributable DirectX SDK and aren't in any modern Windows SDK. Per
# Agent/knowledge_base/modernization_options.md, this module is deferred to the Phase 4 Media
# Foundation rewrite, not something to patch for sub-milestone 1a.
set(WIN32LIB_LIVE_DIRS
    AUDIO DIPTHONG DRAWBUFF FONT IFF KEYBOARD MEM MISC MONO PALETTE
    PLAYCD PROFILE RAWFILE SHAPE TILE TIMER WINCOMM WSA WW_WIN
)

set(WIN32LIB_SOURCES "")
foreach(dir ${WIN32LIB_LIVE_DIRS})
    file(GLOB dir_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*.CPP")
    list(APPEND WIN32LIB_SOURCES ${dir_sources})
endforeach()

# AUDIO/TST.CPP is a broken 3-line test stub (`main {}` isn't valid C++, missing return type and
# parameter list) - dead scratch scaffolding, not real library code, so it's excluded rather than
# "fixed" into something it was never meant to be.
list(REMOVE_ITEM WIN32LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/AUDIO/TST.CPP")

# PLAYCD/REDBOOK.CPP implements methods of RedBookClass, but that class is declared only inside
# PLAYCD.H's "#ifdef NOT_FOR_WIN95" block (DOS real-mode DPMI code: SEGSEL, DPMI_real_alloc,
# union REGS) - a macro this Win32 build never defines. Nothing else in WIN32LIB references
# RedBookClass, so this file was never actually part of the live Win32 library either; excluded
# rather than resurrecting DPMI/real-mode plumbing we've already decided to drop entirely (see
# Agent/knowledge_base/modernization_options.md).
list(REMOVE_ITEM WIN32LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PLAYCD/REDBOOK.CPP")

# RAWFILE/CCFILE.CPP #includes "ccfile.h", which genuinely does not exist anywhere in this GPL
# release (checked, including SRCDEBUG/ and every OLD/ copy) - a real gap in the source drop, not
# a bug to fix here. CODE/ has its own separate CCFILE.H/CCFileClass (see
# Agent/project_state/modules/code/file_io_compression_saveload.md) but its member layout hasn't
# been verified compatible with what this file implements, so borrowing it blind would risk
# silently-wrong behavior. Excluded for now; revisit once CODE/ integration (sub-milestone 1b)
# clarifies how CCFileClass is actually used and whether reconstructing/sharing a header is safe.
list(REMOVE_ITEM WIN32LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/RAWFILE/CCFILE.CPP")

# PROFILE/PROFILE.CPP and PROFILE/WPROFILE.CPP both define identical Start_Profiler(void)/
# Stop_Profiler(void) functions - proven mutually-exclusive alternate implementations, never
# meant to link into the same binary (a genuine ODR violation if both were kept). WPROFILE.CPP's
# callbacks (Old_Profiler_Callback/New_Profiler_Callback) match real ASM-defined symbols in
# APROFILE.ASM; PROFILE.CPP's own callback ("Profiler_Callback") and its "Copy_CHK()" call match
# nothing anywhere in the tree. WPROFILE.CPP is the real, live Windows implementation; PROFILE.CPP
# is excluded as the stale/superseded one.
list(REMOVE_ITEM WIN32LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PROFILE/PROFILE.CPP")

add_library(win32lib STATIC ${WIN32LIB_SOURCES})

# INCLUDE/ must come first: several modules (e.g. MISC/) carry stale duplicate copies of shared
# umbrella headers like WWSTD.H (see Agent/knowledge_base/codebase_history_and_cruft.md) - putting
# INCLUDE/ first in the search order guarantees the canonical, fixed-up header always wins.
target_include_directories(win32lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/INCLUDE"
)
foreach(dir ${WIN32LIB_LIVE_DIRS})
    target_include_directories(win32lib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
endforeach()
