---
name: cmake-vcpkg-reference-patterns
description: Reusable CMake+vcpkg dependency-structuring patterns from a proven reference project, for Phase 0/4 of the modernization plan
metadata:
  type: knowledge_base
---

# CMake + vcpkg Structure — Reference Patterns

The project owner pointed at one of their other, more mature C++/CMake/vcpkg projects as a structural reference for how to wire up CMake + vcpkg here. Inspected 2026-09-12. This file records the transferable patterns for [../plans_wip/modernization_cmake_vcpkg.md](../plans_wip/modernization_cmake_vcpkg.md)'s Phase 0 (build skeleton) and Phase 4 (SDL3 + Vulkan/DX12 + Dear ImGui). That reference project also keeps its own `agent/knowledge_base/`-style documentation tree — the same convention this repo's `Agent/` folder follows, independently validating the approach.

## Directory layout
- vcpkg lives as a **git submodule under `externals/vcpkg`** (not a bare `vcpkg/` at repo root), registered in `.gitmodules`.
- A root `vcpkg.json` manifest, with a `"features"` block grouping optional dependency sets (the reference project's example includes groups like `gui`, `xr`, and other optional-hardware feature sets) — each mapped to a CMake `option()`.
- A root `vcpkg-configuration.json`:
  ```json
  {
      "overlay-ports": ["overlay-ports"],
      "overlay-triplets": ["overlay-triplets"]
  }
  ```
  Simple relative-path arrays, picked up automatically by vcpkg during configure — no CLI flags or CMake cache variables needed to enable them.
- `overlay-ports/<name>/` — custom/patched vcpkg ports for packages where the stock registry port is outdated or missing a needed feature (see the imgui port below).
- `overlay-triplets/` — custom triplet definitions for toolchain combinations vcpkg doesn't ship out of the box (the reference project has one for a clang-cl toolchain).
- `cmake/` holds small, single-purpose included files rather than one giant root `CMakeLists.txt`: `Options.cmake` (options + compiler flags), `FindDependencies.cmake` (all `find_package()` calls), `Dependencies.cmake` (rare `FetchContent`-based deps not in vcpkg at all), plus project-specific ones (fonts/assets/tooling files, not relevant here).

## Root `CMakeLists.txt` ordering (matters)
```cmake
cmake_minimum_required(VERSION 3.30)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Options.cmake")   # BEFORE project()!
project(<name> LANGUAGES CXX)
include(GNUInstallDirs)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindDependencies.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")
add_subdirectory(src)
```
`Options.cmake` must be included **before** `project()` — it's where `option()` calls append to `VCPKG_MANIFEST_FEATURES`, and vcpkg's toolchain file (triggered by `project()`) needs that variable already set to know which manifest features to install.

## The option → vcpkg feature toggle pattern
In `cmake/Options.cmake`:
```cmake
option(BUILD_GUI "BUILD_GUI" ON)
if(BUILD_GUI)
  list(APPEND VCPKG_MANIFEST_FEATURES "gui")
endif()
```
And the matching `vcpkg.json` feature:
```json
"gui": {
  "dependencies": [
    { "name": "imgui", "features": ["sdl3-binding", "vulkan-binding", "docking-experimental"] },
    "vulkan-utility-libraries", "vulkan-memory-allocator",
    { "name": "sdl3", "features": ["vulkan"] }
  ]
}
```
This is the mechanism to reuse for our own optional pieces (e.g. a future `renderer-vulkan` / `renderer-dx12` / `debug-gui` split once Phase 4 starts).

## Header-only libraries with no usable vcpkg CMake config
Some vcpkg ports (miniaudio, autodiff) install headers but no working `find_package()` config. The reference project's workaround, in `cmake/FindDependencies.cmake` — **directly reusable for our own miniaudio dependency**:
```cmake
# miniaudio (single public-domain header, no CMake config shipped by the vcpkg port)
find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)
if(NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio::miniaudio INTERFACE IMPORTED)
    set_target_properties(miniaudio::miniaudio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MINIAUDIO_INCLUDE_DIR}")
endif()
```

## The `imgui` overlay port — already has exactly our Phase 4 bindings
The reference project's `overlay-ports/imgui/vcpkg.json` pins a **newer imgui (1.92.8) than the stock vcpkg registry port**, and its `features` block includes, confirmed by direct inspection: `sdl3-binding`, `vulkan-binding`, **and `dx12-binding`**, alongside `docking-experimental`, `dx9/10/11-binding`, `opengl2/3-binding`, `metal-binding`, `freetype`, `null-backend`, etc. This is precisely the SDL3 + Vulkan + DX12 combination [modernization_options.md](modernization_options.md) calls for in Phase 4 — **plan to vendor/adapt this same overlay port** rather than relying on the stock registry's imgui version, which may lag behind on one of these bindings.

## `CMakePresets.json` patterns
- **Confirms `"Visual Studio 18 2026"` is the correct CMake generator string** for targeting VS2026 (verified by direct inspection of a real working preset, not just documentation).
- Every preset sets `"CMAKE_TOOLCHAIN_FILE": ".../externals/vcpkg/scripts/buildsystems/vcpkg.cmake"`.
- The reference project keeps several presets for different toolchains from one `vcpkg.json`: plain MSVC (`Main`), ClangCL-via-VS-generator (`Clang`), full Clang+Ninja+lld-link (`Clang-Ninja`, using a custom `VCPKG_TARGET_TRIPLET: x64-windows-clang` from `overlay-triplets/`), and even cross-compiling to a mobile ARM target via Ninja + NDK — useful evidence for how far a single vcpkg-based setup scales, though this project's [x64-only decision](../plans_wip/modernization_cmake_vcpkg.md) means we don't need that breadth immediately.
- Feature-gated presets inherit and override: an XR-enabled variant just adds one extra `ON` cache variable on top of the base preset via `"inherits"`.

## Compiler-flag baseline worth copying (from `Options.cmake`)
```cmake
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")
if (WIN32)
  add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
```
`/bigobj` in particular is likely to matter here given how large some `CODE/`/`WIN32LIB/` translation units are (see [../project_state/modules/code/summary.md](../project_state/modules/code/summary.md)).
