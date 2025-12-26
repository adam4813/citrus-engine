# Citrus Scene Editor

A 2D scene editor for the Citrus Engine.

## Features

- **File Menu**: New, Open, Save, Save As scene operations
- **Scene Hierarchy**: View and manage entities in the scene
- **Properties Panel**: Inspect and modify entity properties (placeholder)
- **Viewport**: 2D scene visualization (placeholder)
- **Play/Stop**: Run the scene in the editor

## Building

### Prerequisites

- CMake 3.28+
- vcpkg (with VCPKG_ROOT environment variable set)
- Clang-18+ (Linux) or MSVC 2022 (Windows)

### Build Commands

```bash
# Configure
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux

# Build
cmake --build --preset cli-native-debug
```

### Running

```bash
# Run the editor
./build/cli-native/Debug/citrus-scene-editor
```

## Usage

1. **Create New Scene**: File > New
2. **Add Entities**: Scene > Add Entity
3. **Select Entity**: Click on entity in Hierarchy panel
4. **View Properties**: Selected entity properties shown in Properties panel
5. **Play Scene**: Click Play button in toolbar to run the scene
6. **Save Scene**: File > Save As to save the scene

## Current Status

This is a scaffolded prototype using ImGui for the UI. Future versions will:

- Implement proper scene serialization (loading/saving)
- Add component editing in Properties panel
- Implement 2D rendering in the viewport
- Add drag-and-drop entity creation
- Support undo/redo operations
