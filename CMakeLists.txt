cmake_minimum_required(VERSION 3.28)

# Disable C++20 module scanning for compatibility with GCC/Clang (CMake 3.31+)
# CMP0155: C++ module dependencies are supported by Ninja Generators
if(POLICY CMP0155)
  cmake_policy(SET CMP0155 OLD)
endif()
set(CMAKE_CXX_SCAN_FOR_MODULES OFF CACHE BOOL "Disable C++20 module scanning" FORCE)

# Project configuration
project(game-engine
    VERSION 0.0.1
    DESCRIPTION "Modern C++20 Game Engine"
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
