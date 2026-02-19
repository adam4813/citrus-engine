# Project Setup

This guide explains how to create a standalone game project using Citrus Engine, build it for native platforms, and export it as WebAssembly.

## Quick Start

1. Copy the project template
2. Customize your project settings
3. Build and run

## Creating a New Project

### From Template

Copy the `templates/game-project/` directory to your desired location:

```bash
# Copy template to a sibling directory of the engine
cp -r citrus-engine/templates/game-project ../my-game

# Or on Windows
xcopy /E /I citrus-engine\templates\game-project ..\my-game
```

Your project should be a **sibling** of the engine repository so the vcpkg overlay port can find it:

```
parent-directory/
├── citrus-engine/      # Engine repository
│   ├── ports/          # vcpkg overlay port
│   └── ...
├── vcpkg/              # vcpkg installation
└── my-game/            # Your new project (copied from template)
    ├── assets/
    │   ├── fonts/
    │   ├── scenes/
    │   ├── shaders/
    │   └── textures/
    ├── include/
    ├── src/
    │   └── main.cpp
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── project.json
    ├── vcpkg.json
    └── vcpkg-configuration.json
```

### Customize Your Project

1. **Rename the project** — Update these files with your project name:
   - `CMakeLists.txt`: Change `project(my-game ...)` to your name
   - `vcpkg.json`: Change `"name": "my-game"` to your name
   - `project.json`: Update `name`, `window.title`, and other settings

2. **Update the overlay port path** — Edit `vcpkg-configuration.json` to point to the engine's `ports/` directory:
   ```json
   {
     "overlay-ports": [
       "../citrus-engine/ports"
     ]
   }
   ```
   The path is relative to your project directory and must point to the `ports/` folder inside the engine repository.

3. **Add your source files** — Add `.cpp` files to `src/` and list them in `CMakeLists.txt`:
   ```cmake
   add_executable(
       ${PROJECT_NAME}
       src/main.cpp
       src/my_scene.cpp
       src/player.cpp)
   ```

4. **Add your assets** — Place game assets in the `assets/` subdirectories:
   - `assets/fonts/` — TTF font files
   - `assets/textures/` — PNG/JPG textures
   - `assets/scenes/` — Scene JSON files
   - `assets/shaders/` — Custom shader files

## Project Configuration (project.json)

The `project.json` file defines your game's settings:

```json
{
  "name": "my-game",
  "version": "0.1.0",
  "author": "Your Name",
  "description": "My awesome game",
  "engine": {
    "version": "0.2.x"
  },
  "window": {
    "title": "My Game",
    "width": 1280,
    "height": 720,
    "fullscreen": false,
    "vsync": true
  },
  "startup_scene": "scenes/main.json",
  "assets": {
    "base_path": "assets",
    "directories": ["fonts", "textures", "scenes", "shaders"]
  },
  "build": {
    "targets": [
      {
        "platform": "native",
        "configuration": "Release"
      },
      {
        "platform": "wasm",
        "configuration": "Release",
        "emscripten": {
          "initial_memory_mb": 64,
          "max_memory_mb": 128
        }
      }
    ]
  }
}
```

| Field | Description |
|-------|-------------|
| `name` | Project identifier (must match CMakeLists.txt and vcpkg.json) |
| `version` | Semantic version of your game |
| `engine.version` | Required engine version (informational) |
| `window.*` | Default window settings |
| `startup_scene` | Path to the initial scene (relative to `assets/`) |
| `assets.base_path` | Root directory for game assets |
| `build.targets` | Build configurations for distribution |

> **Note:** The engine does not currently read `project.json` at runtime. It serves as project metadata and will be integrated with the editor's build system in a future release. For now, update `main.cpp` to match your desired window settings.

## Prerequisites

Before building, ensure you have:

- **C++ Compiler**: Visual Studio 2022 (Windows), Clang-18+ (Linux), or Xcode (macOS)
- **CMake**: 3.30 or later
- **Ninja**: Build system (required by presets)
- **vcpkg**: Package manager — clone to a sibling directory of your project
- **Git**: For version control

### Optional (for WebAssembly)

- **Emscripten SDK**: For WASM compilation
- **Python**: For local web server (`python -m http.server`)

## Building

### Environment Setup

```bash
# Set vcpkg root (adjust path to your vcpkg installation)
# Windows
set VCPKG_ROOT=C:\path\to\vcpkg

# Linux/macOS
export VCPKG_ROOT=/path/to/vcpkg
export CC=clang-18
export CXX=clang++-18
```

### Native Build (Windows)

```bash
# Configure (first time — downloads and builds dependencies, may take 5-15 minutes)
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows

# Build
cmake --build --preset cli-native-debug

# Run
./build/cli-native/Debug/my-game.exe
```

On Windows with MSVC, wrap commands in the Visual Studio developer environment:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake --build --preset cli-native-debug'
```

### Native Build (Linux)

```bash
# Configure
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux

# Build
cmake --build --preset cli-native-debug --parallel $(nproc)

# Run
./build/cli-native/Debug/my-game
```

### WebAssembly Build

```bash
# Activate Emscripten
source /path/to/emsdk/emsdk_env.sh

# Configure
cmake --preset wasm -DVCPKG_TARGET_TRIPLET=wasm32-emscripten

# Build
cmake --build --preset wasm-release

# Serve locally
cd build/wasm/Release
python -m http.server 8080
# Open http://localhost:8080/my-game.html
```

## Distribution

### Native Distribution

After a Release build, your distributable files are:

```
build/cli-native/Release/
├── my-game.exe          # (or my-game on Linux)
└── assets/              # Copy from your project
```

Use CMake install to create a clean distribution:

```bash
cmake --install build/cli-native --config Release --prefix dist/native
```

### WebAssembly Distribution

After a WASM Release build:

```
build/wasm/Release/
├── my-game.html         # Entry point
├── my-game.js           # JavaScript glue
├── my-game.wasm         # WebAssembly binary
└── my-game.data         # Packed assets
```

Upload these files to any static web host.

## Adding Dependencies

If your game needs additional libraries (e.g., ImGui for debug UI):

1. Add the dependency to `vcpkg.json`:
   ```json
   {
     "dependencies": [
       "citrus-engine",
       {
         "name": "imgui",
         "features": ["docking-experimental", "glfw-binding", "opengl3-binding"]
       }
     ]
   }
   ```

2. Add `find_package` and link in `CMakeLists.txt`:
   ```cmake
   find_package(imgui CONFIG REQUIRED)
   target_link_libraries(${PROJECT_NAME} PRIVATE citrus-engine::engine-core imgui::imgui)
   ```

3. Reconfigure: `cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows`

## Troubleshooting

### "Could not find citrus-engine"

The vcpkg overlay port path in `vcpkg-configuration.json` must point to the engine's `ports/` directory. Verify the relative path is correct for your project location.

### First build is very slow

The first `cmake --preset` run downloads and builds all dependencies via vcpkg. Subsequent builds use the cached packages.

### Linker errors on Windows

Ensure you're running from a Visual Studio Developer Command Prompt or wrapping commands with `vcvars64.bat`.

### Black screen at runtime

- Verify the camera entity is created with `SetActiveCamera()`
- Check the console for shader compilation errors
- Ensure assets are in the correct directory relative to the executable

## Next Steps

- [Getting Started Guide](getting-started.md) — Engine fundamentals and hello world
- [Architecture Overview](architecture.md) — ECS design and module structure
- [Scene Management](scenes.md) — Creating and loading scenes
- [UI Components](ui-components.md) — Building game UI
