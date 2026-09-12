# Unified Dependency Discovery
#
# Centralizes all find_package() calls so CMakeLists.txt files elsewhere just link against the
# resulting targets. Grows as each modernization phase actually needs a dependency - see
# Agent/plans_wip/modernization_cmake_vcpkg.md for what's expected when.

# --- Phase 1 (DirectDraw compatibility-shim path) ---
find_package(directx-headers CONFIG REQUIRED)

# miniaudio (single public-domain header, no CMake config shipped by the vcpkg port): build the
# INTERFACE target ourselves from the header path once it's added to vcpkg.json in Phase 1.
# find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)
# if(NOT TARGET miniaudio::miniaudio)
#     add_library(miniaudio::miniaudio INTERFACE IMPORTED)
#     set_target_properties(miniaudio::miniaudio PROPERTIES
#         INTERFACE_INCLUDE_DIRECTORIES "${MINIAUDIO_INCLUDE_DIR}")
# endif()

# --- Phase 4 (SDL3 + Vulkan/DX12 + Dear ImGui) - not wired up yet ---
# find_package(SDL3 CONFIG REQUIRED)
# find_package(Vulkan REQUIRED)
# find_package(imgui CONFIG REQUIRED)

# --- Testing ---
if(BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif()
