cmake_minimum_required(VERSION 3.28)

# Project configuration
project(game-engine
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine - Modern C++20 Game Engine"
    LANGUAGES CXX
)

# Set default install location to demo/lib for easy testing when not using vcpkg
if (NOT DEFINED VCPKG_TARGET_TRIPLET AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../game-engine-demo/lib" CACHE PATH "Install prefix" FORCE)
endif ()

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CompilerSettings)
include(Dependencies)

if (NOT EMSCRIPTEN)
    include(StaticAnalysis)
endif ()

# Add subdirectories
add_subdirectory(src/engine)

# Testing setup
include(Testing)

# Install and packaging
include(Install)
