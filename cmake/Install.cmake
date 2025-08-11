# Installation and packaging configuration

# Determine platform suffix for target files
if (EMSCRIPTEN)
    set(PLATFORM_SUFFIX "-wasm")
else ()
    set(PLATFORM_SUFFIX "-native")
endif ()

# Install targets and export them
install(TARGETS engine-core
    EXPORT game-engine-targets${PLATFORM_SUFFIX}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
    FILE_SET CXX_MODULES DESTINATION include/game-engine
)

# Install headers and modules (if needed for consumption)
install(DIRECTORY src/engine/
    DESTINATION include/game-engine
    FILES_MATCHING
    PATTERN "*.cppm"
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Install cmake modules that the package config depends on (only once, not per platform)
install(FILES
    cmake/Emscripten.cmake
    DESTINATION lib/cmake/game-engine
)

# Install assets
install(DIRECTORY assets/
    DESTINATION bin/assets
    FILES_MATCHING
    PATTERN "*.png"
    PATTERN "*.jpg"
    PATTERN "*.jpeg"
    PATTERN "*.vert"
    PATTERN "*.frag"
)

# Export targets for build tree with platform suffix
export(EXPORT game-engine-targets${PLATFORM_SUFFIX}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/game-engine-targets${PLATFORM_SUFFIX}.cmake"
    NAMESPACE game-engine::
)

# Install exported targets with platform suffix
install(EXPORT game-engine-targets${PLATFORM_SUFFIX}
    FILE game-engine-targets${PLATFORM_SUFFIX}.cmake
    DESTINATION lib/cmake/game-engine
    NAMESPACE game-engine::
)

# Configure and install package config files (only for the first platform installed)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/game-engine-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/game-engine-config.cmake"
    INSTALL_DESTINATION lib/cmake/game-engine
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/game-engine-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/game-engine-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/game-engine-config-version.cmake"
    DESTINATION lib/cmake/game-engine
)

# Package configuration
set(CPACK_PACKAGE_NAME "GameEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Game Engine Team")
set(CPACK_PACKAGE_CONTACT "contact@gameengine.dev")

include(CPack)

message(STATUS "Installation and packaging configured for ${CMAKE_SYSTEM_NAME} (${PLATFORM_SUFFIX})")
