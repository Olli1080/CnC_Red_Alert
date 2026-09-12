// Phase 0 build-skeleton smoke test: confirms CMake + vcpkg configure/build/link actually works
// end to end. Not part of the game - safe to delete once Phase 2 introduces real CODE/ and
// WIN32LIB/ targets (see Agent/plans_wip/modernization_cmake_vcpkg.md).
#include <directx/d3d12.h>

#include <cstdio>

int main() {
    std::printf("CMake + vcpkg build skeleton OK (DirectX-Headers resolved).\n");
    return 0;
}
