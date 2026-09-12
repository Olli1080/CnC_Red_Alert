cmake_minimum_required(VERSION 3.30)

# Load options before project() to allow affecting vcpkg features and policies
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Options.cmake")

project(cnc_red_alert LANGUAGES CXX)

include(GNUInstallDirs)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindDependencies.cmake")

# Phase 0 skeleton smoke-test target: proves the build configures, generates, and links against
# a real vcpkg dependency. Replaced by real CODE/ and WIN32LIB/ targets in Phase 2 (see
# Agent/plans_wip/modernization_cmake_vcpkg.md).
add_executable(phase0_smoke_test cmake/phase0_smoke_test.cpp)
target_link_libraries(phase0_smoke_test PRIVATE Microsoft::DirectX-Headers)

if(BUILD_TESTS)
    enable_testing()
endif()
