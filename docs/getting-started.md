# Getting Started

This guide walks you through setting up a new game project using Citrus Engine.

## Prerequisites

Before you begin, ensure you have:

- **C++ Compiler**: Visual Studio 2022 (17.0+) with C++20 support
- **CMake**: 3.28 or later
- **vcpkg**: Package manager for dependencies
- **Git**: For version control

### Optional (for WebAssembly builds)
- **Emscripten SDK (emsdk)**: For WASM compilation
- **Node.js**: For local web server

## Environment Setup

### Install vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

Set the environment variable:

```bash
set VCPKG_ROOT=C:\vcpkg
```

### Install Emscripten (Optional)

For WebAssembly builds:

```bash
git clone https://github.com/emscripten-core/emsdk.git C:\emsdk
cd C:\emsdk
emsdk install latest
emsdk activate latest
```

Set the environment variable:

```bash
set EMSDK=C:\emsdk
```

## Creating Your First Project

### 1. Project Structure

Create your project directory:

```
your-game/
├── src/
│   └── main.cpp
├── assets/
│   ├── shaders/
│   ├── textures/
│   └── fonts/
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

### 2. vcpkg.json

Define your dependencies:

```json
{
  "name": "your-game",
  "version": "1.0.0",
  "dependencies": [
    "citrus-engine"
  ]
}
```

### 3. CMakeLists.txt

Configure your build:

```cmake
cmake_minimum_required(VERSION 3.28)
project(your-game)

# Find Citrus Engine
find_package(citrus-engine CONFIG REQUIRED)

# Create your executable
add_executable(your-game
    src/main.cpp
)

# Link to the engine
target_link_libraries(your-game PRIVATE
    citrus-engine::engine-core
)

# Preload assets for WebAssembly
setup_asset_preload(your-game ${CMAKE_CURRENT_SOURCE_DIR}/assets)
```

### 4. CMakePresets.json

Set up build presets:

```json
{
  "version": 8,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 28,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "default",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/native",
      "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "20",
        "VCPKG_TARGET_TRIPLET": "x64-windows-static"
      }
    }
  ]
}
```

## Hello World Example

Create `src/main.cpp`:

```cpp
import engine;
import glm;

#include <iostream>
#include <GLFW/glfw3.h>

int main() {
    std::cout << "Citrus Engine - Hello World" << std::endl;
    
    // Initialize the engine
    engine::Engine eng;
    if (!eng.Init(1280, 720)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }
    
    glfwSetWindowTitle(eng.window, "My First Citrus Game");
    
    // Create a camera entity
    auto camera = eng.ecs.CreateEntity("MainCamera");
    camera.set<engine::components::Transform>({{0.0f, 0.0f, 5.0f}});
    camera.set<engine::components::Camera>({
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fov = 60.0f,
        .aspect_ratio = 1280.0f / 720.0f,
        .near_plane = 0.1f,
        .far_plane = 100.0f
    });
    eng.ecs.SetActiveCamera(camera);
    
    // Create a simple entity with position
    auto player = eng.ecs.CreateEntity("Player");
    player.set<engine::components::Transform>({{0.0f, 0.0f, 0.0f}});
    
    // Main loop
    float last_time = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(eng.window)) {
        float current_time = static_cast<float>(glfwGetTime());
        float delta_time = current_time - last_time;
        last_time = current_time;
        
        // Begin rendering
        if (eng.renderer) {
            eng.renderer->BeginFrame();
        }
        
        // Update engine systems
        eng.Update(delta_time);
        
        // End rendering
        if (eng.renderer) {
            eng.renderer->EndFrame();
        }
        
        glfwSwapBuffers(eng.window);
        glfwPollEvents();
    }
    
    // Cleanup
    camera.destruct();
    player.destruct();
    eng.Shutdown();
    
    return 0;
}
```

## Building Your Project

### Native Build (Windows)

```bash
# Configure
cmake --preset default

# Build
cmake --build build/native --config Release

# Run
./build/native/Release/your-game.exe
```

### WebAssembly Build

```bash
# Configure for WASM
cmake --preset wasm-emscripten

# Build
cmake --build build/wasm --config Release

# Run local server
cd build/wasm
python -m http.server 8080
# Navigate to http://localhost:8080/your-game.html
```

## Next Steps

Now that you have a basic project running, explore these topics:

- **[Architecture Overview](architecture.md)** - Understand the engine's structure
- **[Physics API](physics.md)** - Add physics simulation to your game
- **[Audio API](audio.md)** - Integrate sound and music
- **[Scene Management](scenes.md)** - Organize your game into scenes
- **[UI Components](ui-components.md)** - Build user interfaces
- **[Tilemap System](tilemap-system.md)** - Create 2D tile-based worlds

## Example Projects

Check out the `examples/` directory for more complete examples:

- **triangle_2d_scene.cpp** - 2D rendering with input
- **cube_3d_scene.cpp** - 3D rendering and transforms
- **ui_showcase_scene.cpp** - UI system demonstration
- **scene_management_scene.cpp** - Scene transitions and lifecycle

## Troubleshooting

### Build Issues

**CMake can't find vcpkg:**
```bash
# Ensure VCPKG_ROOT is set correctly
echo %VCPKG_ROOT%
```

**Missing dependencies:**
```bash
# Clean and rebuild vcpkg cache
vcpkg remove --outdated
vcpkg install
```

**Linker errors:**
- Ensure you're using the correct triplet (`x64-windows-static` for Windows)
- Check that all modules are imported correctly

### Runtime Issues

**Black screen / nothing renders:**
- Verify camera is set up correctly with `SetActiveCamera()`
- Check shader compilation errors in console
- Ensure vertex attribute layouts match shader inputs (see README)

**Asset loading failures:**
- Check asset paths are relative to executable location
- For WASM: Ensure `setup_asset_preload()` is in CMakeLists.txt

**Poor performance:**
- Use Release build configuration
- Enable V-Sync for consistent frame rate
- Profile with Visual Studio Diagnostic Tools

## Getting Help

- **Examples**: See `examples/src/` for working code
- **API Reference**: Browse the [API documentation](api/index.md)
- **Issues**: Report bugs on [GitHub](https://github.com/adam4813/citrus-engine/issues)
