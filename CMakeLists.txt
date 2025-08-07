cmake_minimum_required(VERSION 3.28)

# Project configuration
project(game-engine
    VERSION 0.0.1
    DESCRIPTION "Modern C++20 Game Engine"
    LANGUAGES CXX
)

# Set default install location to demo/lib for easy testing
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../game-engine-demo/lib" CACHE PATH "Install prefix" FORCE)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(Platform)
include(CompilerSettings)
include(Dependencies)

if (PLATFORM_WASM)
    include(Emscripten)
endif ()

if (PLATFORM_NATIVE)
    include(StaticAnalysis)
endif ()

# Add subdirectories
add_subdirectory(src/engine)

# Testing setup
include(Testing)

# Install and packaging
include(Install)
