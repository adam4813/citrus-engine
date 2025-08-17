# Modern C++20 Game Engine

> A high-performance, cross-platform game engine built with modern C++20 features, targeting Windows native and
> WebAssembly platforms.

## Overview

This project implements a modular game engine designed for colony simulation games and general-purpose game development.
The engine features:

- **Modern C++20** with modules, concepts, and coroutines
- **Entity Component System (ECS)** architecture for high performance
- **Cross-platform rendering** via OpenGL ES 2.0/WebGL abstraction
- **Multi-threading support** with job system and frame pipelining
- **WebAssembly support** for browser deployment

## Quick Start

### Prerequisites

#### Windows Development

- **Visual Studio 2022** (17.0 or later) with C++20 support
- **CMake 3.20+**
- **vcpkg** package manager
- **Git** for version control

#### Additional for WebAssembly

- **Emscripten SDK (emsdk)** for WASM compilation
- **Node.js** (optional, for local web server)

### Environment Setup

1. **Install vcpkg** (if not already installed):
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. **Set environment variables**:
   ```bash
   # Add to your system environment variables
   VCPKG_ROOT=C:\vcpkg
   ```

3. **Install Emscripten** (for WASM builds):
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git C:\emsdk
   cd C:\emsdk
   emsdk install latest
   emsdk activate latest
   ```

4. **Set Emscripten environment variable**:
   ```bash
   EMSDK=C:\emsdk
   ```

### Building the Engine

#### Native Windows Build

1. **Configure the build**:
   ```bash
   cmake --preset default
   ```

2. **Build the project**:
   ```bash
   cmake --build build/native --config Release
   ```

3. **Install the engine** (for use in other projects):
   ```bash
   cmake --install build/native --config [Debug|Release]
   ```

4. **Run the MVP demo**:
   ```bash
   ./build/native/Release/game-engine-demo.exe
   ```

#### WebAssembly Build

1. **Configure for WASM**:
   ```bash
   cmake --preset wasm-emscripten
   ```

2. **Build WASM version**:
   ```bash
   cmake --build build/wasm --config Release
   ```

3. **Install the engine** (for use in other projects):
   ```bash
   cmake --install build/wasm --config Release
   ```

4. **Run locally** (using built-in server helper):
   ```bash
   # Use the engine's built-in serve target
   cmake --build build/wasm --target serve-game-engine-demo
   # Navigate to http://localhost:8080/game-engine-demo.html in your browser
   ```

   **Alternative** (manual Python server):
   ```bash
   cd build/wasm
   python -m http.server 8080
   # Navigate to http://localhost:8080 in your browser
   ```

### Emscripten Integration Features

The game engine provides comprehensive Emscripten integration when building for WebAssembly:

#### Automatic Configuration

- **Executable Suffix**: Automatically sets `CMAKE_EXECUTABLE_SUFFIX` to `.html` for WASM builds
- **Link Options**: Applies optimized Emscripten link options including:
    - WebGL 2.0 support (`-sMIN_WEBGL_VERSION=2`, `-sMAX_WEBGL_VERSION=2`)
    - GLFW3 integration (`-sUSE_GLFW=3`)
    - Memory management (`-sALLOW_MEMORY_GROWTH=1`, 64MB initial, 128MB maximum)
    - Filesystem support (`-sFORCE_FILESYSTEM=1`)
    - Debug vs Release optimizations

#### Built-in Helper Functions

- **`add_wasm_serve_target(target_name)`**: Creates a serve target for any WASM executable
    - Starts a Python HTTP server on `localhost:8080`
    - Serves the build directory with proper WASM/HTML files
    - Example: `cmake --build build/wasm --target serve-your-game`

#### Asset Management Helpers

- **`setup_asset_preload(target_name)`**: Automatically configures asset preloading for WASM builds
    - Embeds assets from `assets/` directory into the WASM virtual filesystem
    - Handles empty asset directories gracefully
    - Ensures assets are available at runtime via `/assets/` path

### Quick Build Commands

```bash
# Full clean build and install (Windows)
cmake --preset default && cmake --build build/native --config Release && cmake --install build/native --config Release

# WASM build and install
cmake --preset wasm-emscripten && cmake --build build/wasm --config Release && cmake --install build/wasm --config Release
```

## Using the Engine in Your Project

The game engine is designed to be consumed as a CMake package in other projects. Here's how to integrate it:

### Project Setup

#### 1. Dependencies Configuration

Add the engine's dependencies to your project's `vcpkg.json`:

```json
{
  "name": "your-game-project",
  "version": "1.0.0",
  "dependencies": [
    "flecs",
    "glad",
    "glfw3",
    "glm",
    "gtest",
    {
      "name": "imgui",
      "features": [
        "glfw-binding",
        "opengl3-binding"
      ]
    },
    "nlohmann-json",
    "spdlog",
    "stb"
  ],
  "builtin-baseline": "dd3097e305afa53f7b4312371f62058d2e665320",
  "overrides": [
    {
      "name": "spdlog",
      "version": "1.11.0"
    },
    {
      "name": "fmt",
      "version": "9.0.0"
    }
  ]
}
```

#### 2. CMake Integration

In your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(your-game-project)

# Find the engine library (provides dependencies, platform detection, and compiler settings)
find_package(game-engine CONFIG REQUIRED)

# Create your executable
add_executable(your-game-demo
    src/main.cpp
    # ... your source files
)

# Link to the engine (provides all dependencies transitively)
target_link_libraries(your-game-demo PRIVATE
    game-engine::engine-core
)

if (EMSCRIPTEN)
    # Set the target extension, used with the built-in Emscripten template
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Set memory limits
    set(INITIAL_MEMORY_BYTES 67108864)  # 64MB initial memory
    set(MAXIMUM_MEMORY_BYTES 134217728) # 128MB maximum memory

    # Set Emscripten-specific compile options. Configure these 
    target_link_options(${target_name} PRIVATE
        # The engine is configured with these flags
        #-sMIN_WEBGL_VERSION=2
        #-sMAX_WEBGL_VERSION=2
        #-sUSE_GLFW=3
        #-sSTACK_SIZE=1mb # Ensure it is at least 1mb, otherwise you may get stack overflow errors.

        # Emscripten-specific flags that the engine is also configured with
        #-sALLOW_MEMORY_GROWTH=1
        #-sEXPORTED_RUNTIME_METHODS=cwrap

        # The following flags are configurable and are based on your project needs
        -sWASM=1
        -sFORCE_FILESYSTEM=1
        -sINITIAL_MEMORY=${INITIAL_MEMORY_BYTES}
        -sMAXIMUM_MEMORY=${MAXIMUM_MEMORY_BYTES}
        # Debug vs Release specific flags
        "$<$<CONFIG:Debug>:-sASSERTIONS=1;-sGL_DEBUG=1>"
        "$<$<NOT:$<CONFIG:Debug>>:-O3;--closure 1>"
    )
endif ()

setup_asset_preload(your-game-demo ${CMAKE_CURRENT_SOURCE_DIR}/assets)
```

#### 3. CMake Presets Configuration

Create `CMakePresets.json` in your project root:

```json
{
  "version": 8,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "installDir": "${sourceDir}/install/${presetName}",
      "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "20",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "native",
      "displayName": "Native Build",
      "inherits": "base",
      "cacheVariables": {
        "VCPKG_TARGET_TRIPLET": "x64-windows"
      }
    },
    {
      "name": "wasm",
      "displayName": "WASM Build",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VCPKG_CHAINLOAD_TOOLCHAIN_FILE": "$env{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake",
        "VCPKG_TARGET_TRIPLET": "wasm32-emscripten",
        "EMSCRIPTEN_ROOT_PATH": "$env{EMSDK}/upstream/emscripten"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "native",
      "configurePreset": "native"
    },
    {
      "name": "wasm",
      "configurePreset": "wasm"
    }
  ]
}
```

### Building Your Project

#### Native Build

```bash
# Configure and build
cmake --preset native
cmake --build build/native --config Release

# Run your game
./build/native/your-game-demo
```

#### WebAssembly Build

```bash
# Configure and build
cmake --preset wasm
cmake --build build/wasm --config Release

# Serve locally
cd build/wasm
python -m http.server 8080
```

### Engine Integration Example

```cpp
import engine.platform;
import engine.rendering;
import engine.ecs;

int main() {
    // Initialize the engine platform
    auto platform = engine::Platform::Create();
    
    // Create rendering context
    auto renderer = engine::Renderer::Create();
    
    // Set up ECS world
    auto world = engine::ECS::CreateWorld();
    
    // Your game logic here...
    
    return 0;
}
```

### Prerequisites for Consumer Projects

- **Same as engine development**: Visual Studio 2022, CMake 3.20+, vcpkg, Git
- **For WASM**: Emscripten SDK installed and configured
- **Engine Installation**: The game engine must be built and installed first

### Troubleshooting Consumer Projects

**Issue**: `find_package(game-engine CONFIG REQUIRED)` fails

```bash
# Solution: Ensure the engine is installed to a location CMake can find
# Either install to system location or set CMAKE_PREFIX_PATH
cmake --preset native -DCMAKE_PREFIX_PATH=/path/to/engine/install
```

**Issue**: WASM build fails with missing dependencies

```bash
# Solution: Ensure all engine dependencies are in your vcpkg.json
# and use the exact same triplet (wasm32-emscripten)
```

**Issue**: Module import errors in consumer project

```bash
# Solution: Ensure your project uses C++20 standard and proper module support
# Check that CMAKE_CXX_STANDARD is set to 20 in your CMakeLists.txt
```

## Assets Management

The engine supports loading various asset types for both native and WebAssembly builds.

### Asset Directory Structure

```
assets/
├── shaders/          # GLSL shader files (.vert, .frag)
├── textures/         # Image files (.png, .jpg, .webp)
├── models/           # 3D model files (.obj, .gltf)
├── audio/            # Audio files (.wav, .ogg)
└── fonts/            # Font files (.ttf, .otf)
```

### Platform-Specific Asset Loading

#### Native Builds

- Assets are loaded directly from the filesystem using relative paths from the executable
- Supports hot-reloading during development
- No build-time asset processing required

#### WebAssembly Builds

- Assets are preloaded into the WASM virtual filesystem at build time
- **Automatic Detection**: Build system automatically includes all files in `assets/` directory
- **Empty Directory Handling**: If no assets are present, preloading is skipped (no build errors)
- **Virtual Path**: Assets are available at `/assets/` path in the WASM filesystem
- **Build Size Impact**: All assets increase the WASM download size

### Adding Assets

1. Place asset files in the appropriate `assets/` subdirectory
2. Reference them in code using engine asset loading functions:

```cpp
import engine.rendering;

// Load texture (works in both native and WASM builds)
auto texture_id = texture_manager.LoadTexture("assets/textures/player.png");

// Load shader pair
auto shader_id = shader_manager.LoadShader(
    "assets/shaders/basic.vert", 
    "assets/shaders/basic.frag"
);
```

3. For WASM builds, assets will be automatically included in the next build

### Important Notes

- **Documentation**: Do not place README.md or other documentation files in the `assets/` directory as they will be
  preloaded in WASM builds
- **File Size**: Be mindful of asset sizes for WASM builds as they affect download time
- **Supported Formats**: Use web-compatible formats (PNG, WEBP, OGG) for better WASM compatibility

## Project Structure

```
game-engine/
├── src/                    # Source code
│   ├── engine/            # Core engine modules
│   │   ├── platform/      # Cross-platform abstraction
│   │   ├── ecs/           # Entity Component System
│   │   ├── rendering/     # OpenGL rendering pipeline
│   │   └── scene/         # Transform and spatial systems
│   ├── editor/            # Future: Development tools
│   └── game/              # Future: Game-specific code
├── assets/                # Game assets
│   ├── textures/          # PNG/JPEG texture files
│   └── shaders/           # GLSL shader source files
├── plan/                  # Design documentation
├── build/                 # Build outputs (generated)
│   ├── native/            # Windows/Linux builds
│   ├── wasm/              # WebAssembly builds
│   └── shared-modules/    # Shared C++20 module files for IntelliSense
├── CMakePresets.json      # CMake build configurations
├── vcpkg.json            # Package dependencies
└── README.md             # This file
```

### C++20 Module Support

The engine uses C++20 modules for improved compilation performance and better dependency management. To support
IntelliSense across multiple build presets, the build system automatically copies module definition files to a shared
location:

#### Shared Modules Directory

- **Location**: `build/shared-modules/`
- **Purpose**: Provides a single source of truth for C++20 module files to prevent IntelliSense conflicts
- **Auto-Generated**: Created automatically during CMake configuration
- **Contents**: Module definition files (`.cppm`) from dependencies like GLM

#### How It Works

1. **CMake Configuration**: During the configure step, `cmake/Dependencies.cmake` identifies module files from vcpkg
   packages
2. **Module Detection**: Automatically locates module definition files (e.g., `glm.cppm`) in package directories
3. **Shared Copy**: Copies module files to `build/shared-modules/` to avoid duplicate module declarations
4. **IntelliSense Support**: IDE can reference a single copy of each module, eliminating confusion between
   preset-specific copies

#### Supported Modules

Current auto-copied modules include:

- **GLM**: Mathematical operations (`glm.cppm`)

This process is transparent to developers - no manual intervention required for module management.

## Adding Assets

### Textures

1. **Add texture files** to `assets/textures/`:
   ```
   assets/textures/
   ├── cube_texture.png
   ├── ui_sprite.png
   └── default.png
   ```

2. **Supported formats**: PNG, JPEG (via stb_image)

3. **Recommended texture sizes**: Power of 2 (256x256, 512x512, 1024x1024)

4. **Asset loading** in code:
   ```cpp
   // Texture loading example (implementation-specific)
   auto texture = engine::LoadTexture("assets/textures/cube_texture.png");
   ```

### Shaders

1. **Add shader files** to `assets/shaders/`:
   ```
   assets/shaders/
   ├── basic_3d.vert
   ├── basic_3d.frag
   ├── hud_2d.vert
   └── hud_2d.frag
   ```

2. **Shader compatibility**:
    - **Windows**: OpenGL 3.3+ GLSL 330
    - **WebAssembly**: WebGL 2.0 GLSL ES 300

3. **Cross-platform shader example**:
   ```glsl
   #version 300 es
   precision mediump float;
   
   in vec2 texCoord;
   out vec4 fragColor;
   uniform sampler2D u_texture;
   
   void main() {
       fragColor = texture(u_texture, texCoord);
   }
   ```

## Development Workflow

### MVP Demo Features

The current MVP demonstrates:

- **10 textured cubes** with independent rotation
- **Real-time input** via WASD/arrow keys
- **2D HUD overlay** with sprite rendering
- **Cross-platform compatibility** (Windows + WASM)

### Controls (MVP Demo)

- **WASD**: Move camera/world position
- **Arrow Keys**: Rotate selected objects
- **ESC**: Exit application

### Hot Development Tips

1. **Asset Changes**: Rebuild required for asset updates in MVP
2. **Shader Changes**: Rebuild required for shader updates in MVP
3. **Code Changes**: Standard CMake incremental build
4. **Cross-platform Testing**: Test both native and WASM builds regularly

### Performance Profiling

```bash
# Build with profiling enabled
cmake --build build/native --config RelWithDebInfo

# Profile with your preferred tool
# - Visual Studio Diagnostic Tools
# - Intel VTune
# - Browser DevTools (for WASM)
```

## Architecture Overview

### Engine Modules

The engine is built with a modular, dependency-driven architecture:

#### Foundation Layer

- **engine.platform** - Cross-platform abstractions (timing, file I/O, memory)
- **engine.ecs** - High-performance Entity Component System
- **engine.rendering** - OpenGL ES 2.0/WebGL rendering pipeline
- **engine.scene** - Transform hierarchies and spatial management

#### Future Engine Layer

- **engine.physics** - 2D/3D physics simulation
- **engine.scripting** - Multi-language scripting support
- **engine.animation** - Keyframe and procedural animation
- **engine.networking** - Multiplayer networking
- **engine.profiling** - Performance analysis tools

### Key Design Principles

1. **Data-Oriented Design**: Cache-friendly memory layouts
2. **Cross-Platform First**: Identical behavior on all targets
3. **Thread-Safe Architecture**: Lock-free where possible
4. **Modern C++20**: Concepts, modules, coroutines
5. **Zero-Cost Abstractions**: Performance without compromise

## Troubleshooting

### Common Build Issues

**Issue**: CMake can't find vcpkg

```bash
# Solution: Ensure VCPKG_ROOT environment variable is set
echo %VCPKG_ROOT%  # Should show C:\vcpkg or your installation path
```

**Issue**: Emscripten build fails

```bash
# Solution: Verify EMSDK environment variable
echo %EMSDK%       # Should show your emsdk installation path
emsdk list         # Verify installation
```

**Issue**: Missing OpenGL context

```bash
# Solution: Update graphics drivers and ensure OpenGL 3.3+ support
# For WASM: Ensure WebGL 2.0 support in browser
```

**Issue**: Asset loading failures

```bash
# Solution: Verify asset paths are relative to executable location
# Check that assets/ directory exists in build output
```

### Performance Issues

**Symptom**: Low FPS on Windows

- Check Debug vs Release build configuration
- Verify V-Sync settings
- Profile with Visual Studio Diagnostic Tools

**Symptom**: Slow WASM performance

- Verify Release build with optimizations enabled
- Check browser WebGL implementation
- Monitor browser console for warnings

### Debug Build vs Release Build

```bash
# Debug: Slower, full debugging symbols
cmake --build build/native --config Debug

# Release: Optimized, minimal debug info
cmake --build build/native --config Release

# RelWithDebInfo: Optimized + debugging (best for profiling)
cmake --build build/native --config RelWithDebInfo
```

## Contributing

### Code Style

- **C++20 standard** with modern features preferred
- **snake_case** for variables and functions
- **PascalCase** for types and classes
- **UPPER_CASE** for constants and macros

### Adding New Systems

1. Create system documentation in `plan/systems/`
2. Define module interface in `plan/modules/`
3. Implement in appropriate `src/engine/` subdirectory
4. Add comprehensive unit tests
5. Update this README with new dependencies or build steps

### Testing

```bash
# Run unit tests (when implemented)
ctest --build-dir build/native --config Release

# Manual testing
# 1. Build both native and WASM versions
# 2. Verify identical visual output
# 3. Test all input controls
# 4. Check error handling edge cases
```

## License

[Specify your license here]

## Resources

- **C++20 Reference**: https://en.cppreference.com/w/cpp/20
- **OpenGL ES 2.0 Specification**: https://www.khronos.org/opengles/
- **WebGL 2.0 Specification**: https://www.khronos.org/webgl/
- **Emscripten Documentation**: https://emscripten.org/docs/
- **vcpkg Package Manager**: https://vcpkg.io/

---

**Engine Version**: MVP v1.0  
**Last Updated**: July 2, 2025  
**Target Platforms**: Windows x64, Linux x64, WebAssembly
