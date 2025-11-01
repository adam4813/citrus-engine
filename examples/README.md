# Citrus Engine Examples

This directory contains a unified examples application that demonstrates various features of the Citrus Engine.

## Overview

The examples application provides:

- **Scene-based organization**: Each example is implemented as a self-contained scene
- **Easy scene switching**: ImGui menu for switching between examples at runtime
- **Cross-platform**: Builds for both native (Windows/Linux/macOS) and WebAssembly
- **Command-line control**: Specify which scene to show by default

## Building Examples

### Prerequisites

1. Follow the main Citrus Engine build instructions in the root README.md
2. Ensure vcpkg is set up and `VCPKG_ROOT` is set
3. Ensure the Citrus Engine is built and installed (or available via vcpkg overlay)

### Native Build

```bash
cd examples
export VCPKG_ROOT=/path/to/vcpkg

# Configure
cmake --preset native

# Build
cmake --build --preset native-release

# Run
./build/citrus-examples
```

### WebAssembly Build

```bash
cd examples
export VCPKG_ROOT=/path/to/vcpkg
source /path/to/emsdk/emsdk_env.sh

# Configure
cmake --preset wasm

# Build
cmake --build --preset wasm-release

# Serve (requires Python)
cd build/wasm/Release
python3 -m http.server 8080
# Open http://localhost:8080/citrus-examples.html
```

## Running Examples

### Native

```bash
# Run with default scene
./citrus-examples

# Run with specific scene
./citrus-examples --scene "Hello World"
```

### WebAssembly

Open the HTML file in a web browser. To specify a default scene, add a URL parameter:

```
http://localhost:8080/citrus-examples.html?scene=Hello%20World
```

## Scene Switching

While the application is running, use the **Scenes** menu in the menu bar to switch between available examples.

## Creating a New Example Scene

To add a new example scene:

### 1. Create Your Scene Class

Create a new file (e.g., `examples/src/my_example.cpp`):

```cpp
#include "example_scene.h"
#include "scene_registry.h"

import engine;

#ifndef __EMSCRIPTEN__
#include <imgui.h>
#else
#include <imgui.h>
#endif

class MyExampleScene : public examples::ExampleScene {
public:
    const char* GetName() const override {
        return "My Example";
    }

    const char* GetDescription() const override {
        return "Demonstrates feature X";
    }

    void Initialize(engine::Engine& engine) override {
        // Initialize your scene here
        // Create entities, load assets, etc.
    }

    void Shutdown(engine::Engine& engine) override {
        // Clean up your scene here
    }

    void Update(engine::Engine& engine, float delta_time) override {
        // Update logic here
    }

    void Render(engine::Engine& engine) override {
        // Rendering logic here
    }

    void RenderUI(engine::Engine& engine) override {
        // ImGui UI here
        ImGui::Begin("My Example");
        ImGui::Text("Example UI");
        ImGui::End();
    }
};

// Register the scene
REGISTER_EXAMPLE_SCENE(MyExampleScene, "My Example", "Demonstrates feature X");
```

### 2. Add to CMakeLists.txt

Add your source file to the `citrus-examples` target in `examples/CMakeLists.txt`:

```cmake
add_executable(citrus-examples
    src/main.cpp
    src/example_scene.cpp
    src/scene_registry.cpp
    src/scene_switcher.cpp
    src/my_example.cpp  # Add your file here
)
```

### 3. Build and Run

Rebuild the examples application and your scene will automatically appear in the Scenes menu.

## Architecture

### Key Components

- **ExampleScene**: Base interface for all example scenes
- **SceneRegistry**: Central registry of all available scenes
- **SceneSwitcher**: Manages the active scene and UI for switching
- **REGISTER_EXAMPLE_SCENE()**: Macro for automatic scene registration

### Scene Lifecycle

1. **Registration**: Scenes are registered at program startup via static initialization
2. **Creation**: Scene instance is created when first activated
3. **Initialize()**: Called once when scene becomes active
4. **Update()/Render()**: Called every frame while active
5. **Shutdown()**: Called when switching to a different scene
6. **Destruction**: Scene instance is destroyed

### Best Practices

- Keep scenes self-contained and independent
- Clean up all resources in Shutdown()
- Use the engine's ECS system for entity management
- Provide helpful UI with controls and debug information
- Test on both native and WASM builds

## Asset Management

Example assets are stored in `examples/assets/`:

- **Native builds**: Assets are copied to the build directory
- **WASM builds**: Assets are preloaded into the Emscripten virtual filesystem

Load assets using paths relative to the assets directory:

```cpp
// This works on both native and WASM
auto texture = LoadTexture("assets/textures/example.png");
```

## Troubleshooting

### Scene doesn't appear in menu

- Check that `REGISTER_EXAMPLE_SCENE()` is called
- Ensure the source file is added to CMakeLists.txt
- Verify the scene name is unique

### Assets not loading

- Check asset paths are correct
- Verify assets directory is copied/preloaded
- Check console for error messages

### Build errors

- Ensure Citrus Engine is properly installed
- Check that vcpkg overlay ports are specified
- Verify C++20 compiler support

## Examples Included

- **Hello World**: Basic example scene demonstrating the infrastructure

More examples will be added in future updates to demonstrate:

- Rendering features
- Physics integration
- Audio system
- Input handling
- UI components
- And more!
