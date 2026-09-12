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
