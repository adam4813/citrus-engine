# vcpkg portfile for citrus-engine
# Builds and installs the CMake package from the source tree (overlay port scenario)

# Enforce static-only linkage (WASM only supports static; desired for native too)
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

# Configure
vcpkg_cmake_configure(
    SOURCE_PATH "${CURRENT_PORT_DIR}/../.."
    OPTIONS
    -DBUILD_TESTING=OFF
    -DCITRUS_ENGINE_ASSETS_INSTALL_DIR=share/${PORT}/assets
)

# Build and install
vcpkg_cmake_install()

# Fixup CMake package config to be relocatable under share/${PORT}
vcpkg_cmake_config_fixup(
    PACKAGE_NAME citrus-engine
    CONFIG_PATH lib/cmake/citrus-engine
)

# Copy PDBs on Windows MSVC
vcpkg_copy_pdbs()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# Remove any unused debug include or share directories
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)
