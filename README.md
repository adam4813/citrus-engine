```
    ██████╗██╗████████╗██████╗ ██╗   ██╗███████╗
   ██╔════╝██║╚══██╔══╝██╔══██╗██║   ██║██╔════╝
   ██║     ██║   ██║   ██████╔╝██║   ██║███████╗
   ██║     ██║   ██║   ██╔══██╗██║   ██║╚════██║
   ╚██████╗██║   ██║   ██║  ██║╚██████╔╝███████║
    ╚═════╝╚═╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚══════╝
                                                  
   ███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗
   ██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝
   █████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗  
   ██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝  
   ███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗
   ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝
```

<div align="center">

```
           ___
        ,o88888
     ,o8888888'
   ,88888888'
  ,8888888'
 ,8888888'
,8888888'        🍊 Citrus Power! 🍋
88888888.        Squeeze every drop of
`888888888b.     performance from your
 `88888888888b.  game with C++20
  `888888888888b
   `8888888888888b
    `888888888888888b___
     `88888888888888888888o,
      `888888888888888888888b
        `8888888888888888888'
           `""""""""""""'
```

### 🍋 **A Fresh, Zesty Game Engine for Modern C++** 🍊

_Squeeze maximum performance from modern C++20 with ECS architecture, cross-platform support, and WebAssembly first-class citizenship._

[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-yellow.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20WebAssembly-brightgreen.svg)](#platforms)

[Features](#-core-features) • [Quick Start](#-quick-start) • [Documentation](#-documentation) • [Contributing](#-contributing) • [Community](#-community)

</div>

---

## 🍊 What is Citrus Engine?

**Citrus Engine** is a high-performance, cross-platform game engine built with modern C++20 features. It provides:

- 🍋 **Modern C++20** - Built with modules, concepts, and coroutines from the ground up
- 🍊 **ECS Architecture** - Entity Component System design for optimal performance
- 🎨 **Rich Features** - Advanced rendering pipeline, multi-threading, and data-oriented design
- 🌐 **Cross-Platform** - Native Windows/Linux and WebAssembly with feature parity
- 💚 **Open Source** - Zero license fees, no vendor lock-in, clean architecture with no technical debt

Whether you're building a colony simulation, real-time strategy game, or any performance-critical application, Citrus Engine delivers modern C++20 power for your project.

---

## 🌟 Core Features

<div align="center">

```
    🍋 MODERN C++20           🍊 ECS ARCHITECTURE        🍈 CROSS-PLATFORM
   ╔══════════════╗         ╔══════════════╗          ╔══════════════╗
   ║   Modules    ║         ║   Flecs ECS  ║          ║   Windows    ║
   ║   Concepts   ║  ━━━━━  ║ Data-Oriented║  ━━━━━   ║    Linux     ║
   ║ Coroutines   ║         ║ Cache-Friendly║         ║ WebAssembly  ║
   ╚══════════════╝         ╚══════════════╝          ╚══════════════╝
```

</div>

### ⚡ Key Features

| Feature | Description |
|---------|-------------|
| 🎯 **Modern C++20** | Leverage modules, concepts, coroutines, and ranges for clean, expressive code |
| ⚡ **ECS Architecture** | Data-oriented Entity Component System using [Flecs](https://github.com/SanderMertens/flecs) for maximum performance |
| 🎨 **Cross-Platform Rendering** | OpenGL ES 2.0 / WebGL abstraction for consistent visuals everywhere |
| 🚀 **Multi-Threading** | Job system with frame pipelining for parallel execution |
| 🌐 **WebAssembly First** | Deploy to browsers with full feature parity to native builds |
| 📦 **vcpkg Integration** | Easy dependency management with vcpkg overlay ports |
| 🎮 **ImGui Integration** | Built-in immediate mode GUI for development tools |
| 🗺️ **Tilemap System** | Efficient 2D tile-based rendering for strategy and simulation games |
| 🎯 **Zero License Fees** | Open source with no royalties or subscriptions |

---

## 🚀 Quick Start

### 📋 Prerequisites

Before you begin, ensure you have these tools installed:

#### 🪟 Windows Development
- **Visual Studio 2022** (17.0 or later) with C++20 support
- **CMake 3.20+**
- **vcpkg** package manager
- **Git** for version control

#### 🌐 WebAssembly (Additional)
- **Emscripten SDK (emsdk)** for WASM compilation
- **Node.js** (optional, for local web server)

### ⚙️ Environment Setup

<details>
<summary><b>🔧 Click to expand setup instructions</b></summary>

#### 1. Install vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

Set environment variable:
```bash
VCPKG_ROOT=C:\vcpkg
```

#### 2. Install Emscripten (for WASM builds)

```bash
git clone https://github.com/emscripten-core/emsdk.git C:\emsdk
cd C:\emsdk
emsdk install latest
emsdk activate latest
```

Set environment variable:
```bash
EMSDK=C:\emsdk
```

</details>

### 🏗️ Building Citrus Engine

#### 🪟 Native Windows Build

```bash
# Configure the build
cmake --preset default

# Build the project
cmake --build build/native --config Release

# Install the engine (optional, for use in other projects)
cmake --install build/native --config Release

# Run the demo 🎮
./build/native/Release/game-engine-demo.exe
```

#### 🌐 WebAssembly Build

```bash
# Configure for WASM
cmake --preset wasm-emscripten

# Build WASM version
cmake --build build/wasm --config Release

# Install (optional)
cmake --install build/wasm --config Release

# Run locally with built-in server 🌐
cmake --build build/wasm --target serve-game-engine-demo
# Navigate to http://localhost:8080/game-engine-demo.html
```

### 📦 Installing via vcpkg

Citrus Engine can be installed as a vcpkg port with all dependencies included:

```bash
# Windows (static)
vcpkg install game-engine:x64-windows-static --overlay-ports=ports

# WebAssembly
vcpkg install game-engine:wasm32-emscripten --overlay-ports=ports
```

Use in your CMake project:
```cmake
find_package(game-engine CONFIG REQUIRED)
target_link_libraries(your-target PRIVATE game-engine::engine-core)
```

---

## 🎮 Using Citrus Engine in Your Project

Transform your game development with the power of citrus! Here's how to integrate Citrus Engine into your project.

### 📋 Project Setup

#### 1. Create Your vcpkg.json

```json
{
  "name": "your-awesome-game",
  "version": "1.0.0",
  "dependencies": [
    "game-engine"
  ],
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

> 💡 **Note**: The `overrides` block ensures compatible versions of logging libraries are used.

#### 2. Setup Your CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(your-awesome-game)

# Find Citrus Engine 🍊
find_package(game-engine CONFIG REQUIRED)

# Create your executable
add_executable(your-game
    src/main.cpp
    # ... your source files
)

# Link to the engine 🔗
target_link_libraries(your-game PRIVATE
    game-engine::engine-core
)

# WebAssembly configuration (optional)
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(your-game PRIVATE
        -sWASM=1
        -sFORCE_FILESYSTEM=1
        -sINITIAL_MEMORY=67108864    # 64MB
        -sMAXIMUM_MEMORY=134217728   # 128MB
    )
    setup_asset_preload(your-game ${CMAKE_CURRENT_SOURCE_DIR}/assets)
endif()
```

#### 3. Configure CMake Presets

Create `CMakePresets.json`:

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
      "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "20"
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
        "VCPKG_TARGET_TRIPLET": "wasm32-emscripten"
      }
    }
  ]
}
```

### 💻 Your First Game with Citrus Engine

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

### 🏗️ Building Your Project

```bash
# Native build
cmake --preset native
cmake --build build/native --config Release
./build/native/your-game

# WebAssembly build
cmake --preset wasm
cmake --build build/wasm --config Release
cd build/wasm && python -m http.server 8080
```

---

## 🎨 Assets & Resources

Citrus Engine supports a rich variety of asset types for building beautiful games.

### 📁 Asset Directory Structure

```
assets/
├── shaders/          # 🎨 GLSL shader files (.vert, .frag)
├── textures/         # 🖼️ Image files (.png, .jpg, .webp)
├── models/           # 🗿 3D model files (.obj, .gltf)
├── audio/            # 🔊 Audio files (.wav, .ogg)
└── fonts/            # 📝 Font files (.ttf, .otf)
```

### 🌐 Platform-Specific Asset Loading

#### Native Builds
- Assets loaded directly from filesystem
- Supports hot-reloading during development
- No build-time processing required

#### WebAssembly Builds
- Assets preloaded into WASM virtual filesystem
- Automatic detection from `assets/` directory
- Available at `/assets/` path in runtime

### 📦 Loading Assets in Code

```cpp
import engine.rendering;

// Load texture 🖼️
auto texture_id = texture_manager.LoadTexture("assets/textures/citrus_sprite.png");

// Load shader pair 🎨
auto shader_id = shader_manager.LoadShader(
    "assets/shaders/juicy.vert", 
    "assets/shaders/juicy.frag"
);
```

---

## 🏗️ Architecture & Design

### 🎯 Engine Modules

Citrus Engine follows a clean, modular architecture:

```
🍊 Foundation Layer
├── engine.platform      # Cross-platform abstractions
├── engine.ecs           # Entity Component System
├── engine.rendering     # OpenGL/WebGL pipeline
└── engine.scene         # Transform hierarchies

🍋 Future Modules (Planned)
├── engine.physics       # 2D/3D physics simulation
├── engine.scripting     # Multi-language scripting
├── engine.animation     # Animation systems
├── engine.networking    # Multiplayer support
└── engine.profiling     # Performance tools
```

### 💡 Design Principles

| Principle | Description |
|-----------|-------------|
| 🎯 **Data-Oriented** | Cache-friendly memory layouts for maximum performance |
| 🌐 **Cross-Platform First** | Identical behavior on all target platforms |
| 🔒 **Thread-Safe** | Lock-free architecture where possible |
| 🚀 **Modern C++20** | Concepts, modules, and coroutines throughout |
| 🍊 **Zero-Cost Abstractions** | Performance without compromise |

### 📊 Project Structure

```
citrus-engine/
├── src/                    # 🍊 Source code
│   └── engine/            # Core engine modules
│       ├── platform/      # Platform abstraction
│       ├── ecs/           # Entity Component System
│       ├── rendering/     # Rendering pipeline
│       └── scene/         # Scene management
├── assets/                # 🎨 Game assets
├── plan/                  # 📋 Design documentation
├── docs/                  # 📚 Additional documentation
├── tests/                 # 🧪 Unit tests
├── cmake/                 # 🔧 CMake modules
└── ports/                 # 📦 vcpkg port definitions
```

---

## 🧪 Development & Testing

### 🎮 MVP Demo Features

Our current MVP demonstrates the power of Citrus Engine:

- ✅ **10 textured cubes** with independent rotation
- ✅ **Real-time input** via WASD/arrow keys
- ✅ **2D HUD overlay** with sprite rendering
- ✅ **Cross-platform** (Windows + WebAssembly)

### 🎹 Demo Controls

| Key | Action |
|-----|--------|
| **WASD** | Move camera/world position |
| **Arrow Keys** | Rotate selected objects |
| **ESC** | Exit application |

### 🔧 Development Workflow

```bash
# Quick rebuild and test
cmake --build build/native --config Release && ./build/native/Release/game-engine-demo.exe

# Test WASM build locally
cmake --build build/wasm --target serve-game-engine-demo
```

### 🧪 Running Tests

```bash
# Run unit tests (when implemented)
ctest --build-dir build/native --config Release

# Performance profiling
cmake --build build/native --config RelWithDebInfo
```

---

## 🔧 Troubleshooting

Common issues and solutions:

### 🛠️ Common Build Issues

<details>
<summary><b>❌ CMake can't find vcpkg</b></summary>

```bash
# Solution: Ensure VCPKG_ROOT environment variable is set
echo %VCPKG_ROOT%  # Should show C:\vcpkg or your installation path

# Set it if missing:
set VCPKG_ROOT=C:\vcpkg
```
</details>

<details>
<summary><b>❌ Emscripten build fails</b></summary>

```bash
# Solution: Verify EMSDK environment variable
echo %EMSDK%       # Should show your emsdk installation path
emsdk list         # Verify installation

# Ensure latest version is activated
emsdk activate latest
```
</details>

<details>
<summary><b>❌ Missing OpenGL context</b></summary>

```bash
# Native: Update graphics drivers and ensure OpenGL 3.3+ support
# WASM: Ensure WebGL 2.0 support in browser (use Chrome/Firefox/Edge)
```
</details>

<details>
<summary><b>❌ Asset loading failures</b></summary>

```bash
# Verify asset paths are relative to executable location
# Check that assets/ directory exists in build output
# For WASM: Ensure setup_asset_preload() is called in CMakeLists.txt
```
</details>

### ⚡ Performance Issues

**🐌 Low FPS on Windows**
- ✅ Check Debug vs Release build configuration
- ✅ Verify V-Sync settings
- ✅ Profile with Visual Studio Diagnostic Tools

**🐌 Slow WASM Performance**
- ✅ Verify Release build with optimizations enabled
- ✅ Check browser WebGL implementation (prefer Chrome/Firefox)
- ✅ Monitor browser console for warnings
- ✅ Reduce asset sizes for faster download

### 🎯 Build Configuration Guide

```bash
# 🐛 Debug: Slower, full debugging symbols
cmake --build build/native --config Debug

# 🚀 Release: Optimized, minimal debug info
cmake --build build/native --config Release

# 🔍 RelWithDebInfo: Optimized + debugging (best for profiling)
cmake --build build/native --config RelWithDebInfo
```

---

## 🤝 Contributing

We'd love your help making Citrus Engine even more refreshing! 🍊

### 💻 Code Style

Follow the **Citrus Code Style** guidelines:

- **C++20 standard** with modern features preferred
- **snake_case** for variables and functions
- **PascalCase** for types and classes
- **UPPER_CASE** for constants and macros
- See [CODE_STYLE_GUIDE.md](CODE_STYLE_GUIDE.md) for detailed guidelines

### 🌱 Adding New Systems

1. **📋 Plan**: Create system documentation in `plan/systems/`
2. **🎯 Design**: Define module interface in `plan/modules/`
3. **💻 Implement**: Add code in appropriate `src/engine/` subdirectory
4. **🧪 Test**: Add comprehensive unit tests
5. **📝 Document**: Update README and relevant docs

### 🧪 Testing Guidelines

```bash
# Run unit tests
ctest --build-dir build/native --config Release

# Manual testing checklist
# ✅ Build both native and WASM versions
# ✅ Verify identical visual output
# ✅ Test all input controls
# ✅ Check error handling edge cases
```

### 📝 Contribution Process

1. 🍴 Fork the repository
2. 🌿 Create a feature branch (`git checkout -b feature/amazing-feature`)
3. 💻 Make your changes following the code style
4. ✅ Test thoroughly (native + WASM)
5. 📝 Commit with clear messages (`git commit -m 'Add amazing feature'`)
6. 🚀 Push to your branch (`git push origin feature/amazing-feature`)
7. 🎉 Open a Pull Request

---

## 🌟 Community & Support

### 💬 Getting Help

- 📚 **Documentation**: Check our comprehensive docs in the `/docs` folder
- 💡 **Issues**: Open an issue on GitHub for bugs or feature requests
- 🤝 **Discussions**: Join conversations in GitHub Discussions

### 🙏 Acknowledgments

Citrus Engine is powered by amazing open-source libraries:

- 🎯 **[Flecs](https://github.com/SanderMertens/flecs)** - High-performance ECS
- 🎨 **[ImGui](https://github.com/ocornut/imgui)** - Immediate mode GUI
- 🌐 **[GLFW](https://www.glfw.org/)** - Cross-platform windowing
- 📐 **[GLM](https://github.com/g-truc/glm)** - Mathematics library
- 📦 **[vcpkg](https://vcpkg.io/)** - Package management

### 🎖️ Contributors

Thank you to all our contributors who help make Citrus Engine better!

---

## 📚 Resources & Documentation

### 🔗 Essential Links

| Resource | Description |
|----------|-------------|
| 📖 **[C++20 Reference](https://en.cppreference.com/w/cpp/20)** | Modern C++ language features |
| 🎨 **[OpenGL ES 2.0](https://www.khronos.org/opengles/)** | Graphics API specification |
| 🌐 **[WebGL 2.0](https://www.khronos.org/webgl/)** | Web graphics API |
| ⚙️ **[Emscripten Docs](https://emscripten.org/docs/)** | WebAssembly compilation |
| 📦 **[vcpkg](https://vcpkg.io/)** | C++ package manager |
| 🎯 **[Flecs Documentation](https://www.flecs.dev/flecs/)** | ECS framework guide |

### 📁 Additional Documentation

- 📋 **[Code Style Guide](CODE_STYLE_GUIDE.md)** - Coding standards and conventions
- 🗺️ **[Tilemap System](docs/tilemap-system.md)** - 2D tile rendering documentation
- 🏗️ **[Architecture](plan/ARCH_ENGINE_CORE_v1.md)** - Engine architecture overview
- 📊 **[System Status](plan/ENGINE_SYSTEM_STATUS_TREE.md)** - Current implementation status

---

## 📜 License

[Specify your license here]

---

---

<div align="center">

```
        🍊 Citrus Engine 🍋
        
     Built with Modern C++20
     
   ════════════════════════════════════════
```

**Citrus Engine** - _High-Performance Game Development with Modern C++20_

---

**Engine Version**: 🍊 Citrus MVP v1.0  
**Last Updated**: October 28, 2024  
**Target Platforms**: 🪟 Windows x64 | 🐧 Linux x64 | 🌐 WebAssembly

**Made with 🍋 by the Citrus Engine Team**

[⭐ Star us on GitHub](https://github.com/adam4813/citrus-engine) • [🍴 Fork](https://github.com/adam4813/citrus-engine/fork) • [🐛 Report Bug](https://github.com/adam4813/citrus-engine/issues) • [💡 Request Feature](https://github.com/adam4813/citrus-engine/issues)

</div>
