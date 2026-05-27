set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Windows)

# Per-port builds (the inner CMake invocations vcpkg spawns to build each
# port) read VCPKG_CHAINLOAD_TOOLCHAIN_FILE from the triplet, NOT from the
# outer CMakePresets cache. CMAKE_CURRENT_LIST_DIR is the overlay directory
# at triplet-evaluation time, so resolve clang-cl.cmake relative to it.
get_filename_component(_zknt_toolchain
    "${CMAKE_CURRENT_LIST_DIR}/../toolchains/clang-cl.cmake" ABSOLUTE)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${_zknt_toolchain}")
