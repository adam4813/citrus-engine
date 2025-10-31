cmake_minimum_required(VERSION 3.28)

# Examples project configuration
project(citrus-examples
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine Examples - Feature Demonstrations"
    LANGUAGES CXX
)

# C++20 Standard (required for modules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find the Citrus Engine via vcpkg (uses overlay port from ../ports)
find_package(game-engine CONFIG REQUIRED)

# Examples application
add_executable(citrus-examples
    src/main.cpp
    src/example_scene.cpp
    src/scene_registry.cpp
    src/scene_switcher.cpp
)

target_include_directories(citrus-examples PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link to the engine (provides all dependencies transitively)
target_link_libraries(citrus-examples PRIVATE
    game-engine::engine-core
)

# Setup asset handling
setup_asset_preload(citrus-examples "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Platform-specific configuration
if(EMSCRIPTEN)
    # WASM-specific settings
    set_target_properties(citrus-examples PROPERTIES
        SUFFIX ".html"
    )
    
    target_link_options(citrus-examples PRIVATE
        -sWASM=1
        -sFORCE_FILESYSTEM=1
        -sINITIAL_MEMORY=67108864      # 64MB
        -sMAXIMUM_MEMORY=134217728     # 128MB
        -sALLOW_MEMORY_GROWTH=1
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets
    )
else()
    # Native build - copy assets to output directory
    add_custom_command(TARGET citrus-examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:citrus-examples>/assets
        COMMENT "Copying example assets to build directory"
    )
endif()

# Install target
install(TARGETS citrus-examples
    RUNTIME DESTINATION bin
)

if(NOT EMSCRIPTEN)
    install(DIRECTORY assets/
        DESTINATION bin/assets
        PATTERN "*"
    )
endif()
