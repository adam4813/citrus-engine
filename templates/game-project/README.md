# Citrus Engine — Game Project Template

This is a starter template for building a game with [Citrus Engine](https://github.com/adam4813/citrus-engine).

## Quick Start

1. Copy this directory to create your project
2. Update the project name in `CMakeLists.txt`, `vcpkg.json`, and `project.json`
3. Update the overlay port path in `vcpkg-configuration.json`
4. Build and run

See the full [Project Setup Guide](../../docs/project-setup.md) for detailed instructions.

## Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Build configuration (native + WASM) |
| `CMakePresets.json` | Build presets for native, CLI, and WASM |
| `vcpkg.json` | Dependency manifest |
| `vcpkg-configuration.json` | vcpkg overlay port configuration |
| `project.json` | Game project settings (name, window, scenes) |
| `src/main.cpp` | Minimal game loop scaffold |
| `assets/` | Game assets (fonts, textures, scenes, shaders) |
| `include/` | Project header files |
