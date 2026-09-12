# Project Options and Compiler Settings
#
# Must be included BEFORE project() in the root CMakeLists.txt: option() calls here append to
# VCPKG_MANIFEST_FEATURES, and vcpkg's toolchain file (triggered during project()) needs that
# variable already set to know which optional vcpkg.json features to install.

# x64-only project (see Agent/knowledge_base/modernization_options.md, decision #2) - fail fast
# on any other target architecture rather than silently producing a broken build.
if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "" AND NOT CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
    message(FATAL_ERROR "This project targets x64 only (CMAKE_GENERATOR_PLATFORM was '${CMAKE_GENERATOR_PLATFORM}').")
endif()

option(BUILD_TESTS "Build unit tests" OFF)

# Compiler options
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()
