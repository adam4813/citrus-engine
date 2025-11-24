# Installation and packaging configuration

# Determine platform suffix for target files
if (EMSCRIPTEN)
    set(PLATFORM_SUFFIX "-wasm")
else ()
    set(PLATFORM_SUFFIX "-native")
endif ()

# Allow callers (like vcpkg) to control where assets are installed; default to bin/assets
set(CITRUS_ENGINE_ASSETS_INSTALL_DIR "bin/assets" CACHE PATH "Install dir for engine assets")

# Install targets and export them
install(TARGETS engine-core
    EXPORT citrus-engine-targets${PLATFORM_SUFFIX}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
    FILE_SET CXX_MODULES DESTINATION include/citrus-engine
)

# Install headers and modules (if needed for consumption)
install(DIRECTORY src/engine/
    DESTINATION include/citrus-engine
    FILES_MATCHING
    PATTERN "*.cppm"
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Install assets (configurable location) only if the assets directory exists
if (EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY assets/
        DESTINATION ${CITRUS_ENGINE_ASSETS_INSTALL_DIR}
        FILES_MATCHING
        PATTERN "*.png"
        PATTERN "*.jpg"
        PATTERN "*.jpeg"
        PATTERN "*.vert"
        PATTERN "*.frag"
        PATTERN "*.glsl"
    )
endif ()

# Export targets for build tree with platform suffix
export(EXPORT citrus-engine-targets${PLATFORM_SUFFIX}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/citrus-engine-targets${PLATFORM_SUFFIX}.cmake"
    NAMESPACE citrus-engine::
)

# Install exported targets with platform suffix
install(EXPORT citrus-engine-targets${PLATFORM_SUFFIX}
    FILE citrus-engine-targets${PLATFORM_SUFFIX}.cmake
    DESTINATION lib/cmake/citrus-engine
    NAMESPACE citrus-engine::
)

# Configure and install package config files (only for the first platform installed)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/citrus-engine-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/citrus-engine-config.cmake"
    INSTALL_DESTINATION lib/cmake/citrus-engine
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/citrus-engine-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/citrus-engine-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/citrus-engine-config-version.cmake"
    DESTINATION lib/cmake/citrus-engine
)

# Package configuration
set(CPACK_PACKAGE_NAME "CitrusEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Citrus Engine Team")
set(CPACK_PACKAGE_CONTACT "contact@gameengine.dev")

include(CPack)

message(STATUS "Installation and packaging configured for ${CMAKE_SYSTEM_NAME} (${PLATFORM_SUFFIX})")
