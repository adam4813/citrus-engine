# MVP_REQUIREMENTS_v1

> **Minimum Viable Product Requirements for Modern C++20 Game Engine**

## Executive Summary

This document defines the specific requirements for the MVP (Minimum Viable Product) version of the C++20 game engine. The MVP focuses on demonstrating core engine capabilities through a simple 3D demo featuring textured, rotating cubes with a 2D HUD overlay, running on both Windows native and WebAssembly platforms.

**MVP Success Criteria**: Visually compelling demo that proves the engine's cross-platform 3D rendering capabilities with real-time input and ECS architecture.

## MVP Scope Definition

### Core Systems Required

The MVP requires these **4 foundation systems** from the 8 documented foundation modules:

1. **engine.platform** - Cross-platform abstraction (file I/O, timing, memory)
2. **engine.ecs** - Entity Component System architecture  
3. **engine.rendering** - OpenGL ES 2.0/WebGL rendering pipeline
4. **engine.scene** - Transform hierarchies and spatial organization

### Additional MVP Components

5. **Simplified Input** - Basic keyboard input (arrows + WASD) without full action mapping
6. **Basic Asset Loading** - File-based texture loading using stb_image
7. **Hardcoded Geometry** - Cube mesh data embedded in code

## Demo Specification

### 3D Scene Requirements

**Scene Content**:
- 10 textured cubes positioned in 3D space
- Each cube can rotate independently based on user input
- Perspective projection camera system
- Camera movement via inverse world transformation

**Rendering Features**:
- Textured cube rendering using OpenGL ES 2.0/WebGL shader
- Perspective projection matrix
- Basic texture mapping (PNG/JPEG support via stb_image)
- Z-buffer depth testing

**Camera System**:
- Fixed camera position/orientation
- World movement via inverse transformation (camera appears to move)
- Perspective projection (60° FOV recommended)

### 2D HUD Requirements

**HUD Content**:
- Simple sprites rendered as overlay
- Normalized coordinate system (0.0-1.0) for viewport independence
- Separate shader pipeline from 3D rendering

**HUD Features**:
- Orthographic projection for 2D elements
- Alpha blending support for transparent sprites
- Independent of 3D scene transformations

### Input System Requirements

**Supported Input**:
- **Arrow Keys**: Object rotation around X/Y axes
- **WASD**: World/camera movement (inverse transform)
- **Real-time input**: Smooth, continuous movement (not discrete key presses)

**Input Handling**:
- Polling-based input system for MVP
- Direct key state queries (no action mapping complexity)
- Platform abstraction for Windows/WASM input differences

## Technical Architecture

### Project Structure
```
D:\src\game-engine\
├── src\
│   ├── engine\          # Core engine modules
│   │   ├── platform\    # Cross-platform abstraction
│   │   ├── ecs\         # Entity Component System
│   │   ├── rendering\   # OpenGL rendering pipeline
│   │   └── scene\       # Transform and spatial systems
│   ├── editor\          # Future editor code (empty for MVP)
│   └── game\            # Future game-specific code (empty for MVP)
├── assets\              # Texture and asset files
│   ├── textures\        # PNG/JPEG texture files
│   └── shaders\         # GLSL shader source files
├── CMakePresets.json    # Build configuration
└── vcpkg.json          # Dependency management
```

### ECS Component Architecture

**Required Components**:
```cpp
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct Renderable {
    uint32_t mesh_id;
    uint32_t texture_id;
    uint32_t shader_id;
};

struct RotationController {
    glm::vec3 rotation_speed{0.0f};  // radians per second
};
```

**Entity Creation**:
- 10 cube entities created programmatically
- Each entity has Transform, Renderable, and RotationController components
- Positioned in a simple grid or circular pattern

### Rendering Pipeline

**3D Rendering**:
- Vertex shader: MVP transformation with texture coordinates
- Fragment shader: Texture sampling with basic lighting (optional)
- Render command generation from ECS queries
- Batch rendering for efficiency

**2D HUD Rendering**:
- Separate orthographic projection shader
- Normalized coordinate input (0.0-1.0 screen space)
- Alpha blending for transparent elements
- Rendered after 3D scene (depth testing disabled)

### Asset Pipeline

**Texture Loading**:
- File-based loading from `assets/textures/` directory
- stb_image library for PNG/JPEG support
- Basic texture resource management
- No hot-reload for MVP

**Geometry Data**:
- Hardcoded cube vertex data (positions, normals, UVs)
- No mesh file loading for MVP
- Simple vertex buffer management

## Platform Requirements

### Windows Native Build

**Requirements**:
- OpenGL 3.3+ context creation
- Window management via GLFW or similar
- File system access for texture loading
- High-resolution timing

**Build System**:
- CMake with vcpkg for dependencies
- Visual Studio 2022 compiler support
- C++20 modules compilation

### WebAssembly Build

**Requirements**:
- WebGL 2.0 context (OpenGL ES 3.0 equivalent)
- Emscripten toolchain compilation
- Browser-compatible file loading
- Identical visual output to Windows build

**Deployment**:
- Local HTML file execution
- Embedded assets or fetch-based loading
- No specific performance targets for MVP

## Error Handling Strategy

### Graceful Error Handling
- Texture loading failures (fallback to default texture)
- Shader compilation errors (log and continue with basic shader)
- Input system failures (disable input, continue rendering)

### Critical Assertions
- OpenGL context creation failure
- Essential shader compilation failure
- Memory allocation failures
- Platform system initialization failures

## Success Criteria

### Visual Requirements
- 10 textured cubes visible and rotating smoothly
- HUD elements properly overlaid on 3D scene
- Identical visual output between Windows and WASM builds
- Smooth real-time input response

### Technical Requirements
- Consistent 60 FPS performance (no specific target, but smooth)
- Error-free compilation on both platforms
- Clean shutdown without memory leaks
- Proper resource cleanup

### Development Workflow
- Simple build process: `cmake --build --preset default`
- WASM build: `cmake --build --preset wasm-emscripten`
- Asset changes reflected after rebuild
- Clear error messages for common issues

## Out of Scope for MVP

### Foundation Systems Not Required
- **engine.audio** - No sound needed for visual demo
- **engine.input** (full system) - Simple keyboard polling sufficient
- **engine.assets** (full system) - Basic file loading sufficient
- **engine.threading** - Single-threaded execution acceptable

### Engine Systems Deferred
- **engine.physics** - No collision or physics simulation
- **engine.scripting** - Hardcoded behavior sufficient
- **engine.animation** - Basic rotation in update loop
- **engine.networking** - Not applicable to single-player demo
- **engine.profiling** - Manual performance validation

### Advanced Features
- Multiple texture formats beyond PNG/JPEG
- Complex mesh loading (.obj, .fbx files)
- Advanced lighting models
- Post-processing effects
- Audio integration
- Controller input
- Hot-reload development features

## Implementation Priority

### Phase 1: Foundation (Week 1-2)
1. Project structure and CMake setup
2. Platform abstraction layer
3. Basic ECS implementation
4. OpenGL context creation

### Phase 2: Rendering Core (Week 3-4)
1. Basic rendering pipeline
2. Shader loading and compilation
3. Texture loading with stb_image
4. Hardcoded cube geometry

### Phase 3: Demo Integration (Week 5-6)
1. ECS entity creation and management
2. Input system integration
3. HUD rendering system
4. WASM build integration

### Phase 4: Polish and Validation (Week 7-8)
1. Cross-platform testing
2. Error handling implementation
3. Performance validation
4. Documentation and README updates

This MVP provides a solid foundation for demonstrating the engine's capabilities while keeping scope manageable for rapid development and validation.
