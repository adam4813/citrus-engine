# Module Overview

> **C++20 Module Architecture for Colony Game Engine**

## Module Documentation Purpose

This document defines the comprehensive C++20 module architecture that translates the completed engine system
documentation into implementable modules. With all 13 critical engine systems documented (8 foundation + 5 engine
layer), the module planning provides the concrete implementation roadmap for transforming system designs into working
C++20 modules.

## Module Architecture Strategy

### **Three-Layer Module Architecture**

Based on the ENGINE_SYSTEM_STATUS_TREE.md, our modules follow a clear dependency hierarchy:

```
┌─────────────────────────────────────────┐
│         Game-Layer Modules              │
│                                         │
│  colony.ai │ colony.pathfinding         │
│  colony.simulation │ colony.ui          │
│  colony.persistence                     │
│                                         │
│  All import engine.* modules            │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│      Engine-Layer Modules (5/5)        │
│                                         │
│  engine.physics │ engine.scripting      │
│  engine.animation │ engine.networking   │
│  engine.profiling                       │
│                                         │
│  All import foundation modules          │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│     Foundation Modules (8/8)            │
│                                         │
│  engine.platform │ engine.ecs           │
│  engine.threading │ engine.rendering    │
│  engine.audio │ engine.input            │
│  engine.assets │ engine.scene           │
│                                         │
│  Core engine infrastructure             │
└─────────────────────────────────────────┘
```

## Module Naming Conventions

### **Foundation Modules** (Complete)

- `engine.platform` - Cross-platform utilities (file I/O, timing, memory)
- `engine.ecs` - Entity Component System core
- `engine.threading` - Job system and parallel execution
- `engine.rendering` - Graphics abstraction (OpenGL ES 2.0/WebGL)
- `engine.audio` - 3D spatial audio (OpenAL/Web Audio)
- `engine.input` - Cross-platform input with action mapping
- `engine.assets` - Asset loading and hot-reload
- `engine.scene` - Scene management and spatial organization

### **Engine-Layer Modules** (Complete)

- `engine.physics` - 2D physics simulation with spatial optimization
- `engine.scripting` - Multi-language scripting (Lua/Python/AngelScript)
- `engine.animation` - 2D/3D animation with state machines
- `engine.networking` - Cross-platform network infrastructure
- `engine.profiling` - Development and optimization tools

### **Game-Layer Modules** (Future)

- `colony.ai` - AI decision making (Utility AI, GOAP, behavior trees)
- `colony.pathfinding` - A* navigation and obstacle avoidance
- `colony.simulation` - Colony mechanics (resources, jobs, economy)
- `colony.ui` - Player interface (ImGui integration)
- `colony.persistence` - Save/load system

## C++20 Module Benefits

### **Compilation Performance**

- **Faster Builds**: Modules eliminate repetitive header parsing
- **Incremental Compilation**: Module changes only rebuild dependents
- **Parallel Module Compilation**: Independent modules compile simultaneously
- **Binary Module Interface (BMI)**: Pre-compiled module interfaces accelerate builds

### **Dependency Management**

- **Clear Dependencies**: Explicit `import` statements show exact dependencies
- **Encapsulation**: Implementation details hidden from importers
- **Circular Dependency Prevention**: Module system prevents circular imports
- **Interface Stability**: Changes to implementation don't affect importers

### **Development Workflow**

- **Hot-Reload Integration**: Module boundaries enable efficient code reloading
- **Testing Isolation**: Each module independently unit testable
- **Parallel Development**: Teams work on different modules without conflicts
- **Debug Simplification**: Module boundaries simplify debugging scope

## Module Documentation Standards

Each module document follows this structure:

### **1. Module Interface Definition**

```cpp
export module engine.platform;

export namespace engine::platform {
    // Public API exports
    class FileSystem;
    class Timer;
    // ...
}
```

### **2. Implementation Structure**

- Primary Interface: `src/engine/platform/platform.cppm`
- Implementation Files: `src/engine/platform/*.cpp`
- Platform-Specific: Conditional compilation or separate files

### **3. Module Dependencies**

```cpp
// No imports for foundation modules like platform
// or
import engine.platform;  // For higher-level modules
import std.core;          // Standard library modules
```

### **4. CMake Integration**

```cmake
add_library(engine_platform)
target_sources(engine_platform
        PUBLIC FILE_SET cxx_modules FILES
        src/engine/platform/platform.cppm
        PRIVATE
        src/engine/platform/file_system.cpp
        src/engine/platform/timer.cpp
        # Platform-specific implementations
)
```

### **5. Testing Strategy**

- Unit Tests: `tests/engine/platform/`
- Integration Tests: Cross-module functionality
- Performance Tests: Module-specific benchmarks

## Build System Integration

### **CMake C++20 Module Support**

Our CMakeLists.txt will be updated to support:

- `CMAKE_CXX_STANDARD 20` with module support
- `FILE_SET cxx_modules` for module interface files
- Proper module dependency tracking
- Cross-platform module compilation (MSVC, GCC, Clang)

### **Module Compilation Order**

Based on dependency analysis:

1. **Foundation Layer**: platform → ecs, threading, rendering, audio, input, assets, scene
2. **Engine Layer**: physics, scripting, animation, networking, profiling
3. **Game Layer**: ai, pathfinding, simulation, ui, persistence

### **WebAssembly Module Support**

- Emscripten C++20 module compatibility
- Module bundling for web deployment
- WebAssembly-specific module optimizations

## Cross-Platform Module Strategy

### **Platform Abstraction Pattern**

```cpp
// engine.platform module provides unified interface
export module engine.platform;

#ifdef _WIN32
    import :windows_impl;
#elif defined(__linux__)
    import :linux_impl;
#elif defined(__EMSCRIPTEN__)
    import :webassembly_impl;
#endif

export namespace engine::platform {
    // Unified cross-platform API
}
```

### **Implementation Strategy**

- **Unified Interface**: Same module interface across all platforms
- **Platform Partitions**: Platform-specific implementations as module partitions
- **Conditional Compilation**: Platform detection within modules
- **Feature Detection**: Runtime capability detection where needed

## Module Development Workflow

### **Implementation Priority**

1. **Start with Foundation**: `engine.platform` (no dependencies)
2. **Build Dependencies**: `engine.ecs`, `engine.threading` (depend on platform)
3. **Add Multimedia**: `engine.rendering`, `engine.audio`, `engine.input`
4. **Complete Foundation**: `engine.assets`, `engine.scene`
5. **Engine Layer**: `engine.physics`, `engine.scripting`, etc.

### **Development Guidelines**

- **Single Module Focus**: Complete one module before starting another
- **Test-Driven**: Write module tests alongside implementation
- **Interface First**: Design public API before implementation details
- **Documentation**: Update module docs as implementation evolves

## Integration with Existing Codebase

### **Migration Strategy**

- **Gradual Migration**: Convert existing code to modules incrementally
- **Header Compatibility**: Maintain header fallbacks during transition
- **Testing Continuity**: Ensure existing tests continue working
- **Build System Evolution**: Update CMake gradually for module support

### **Existing Code Integration**

- Current `src/engine/` directory maps to foundation modules
- Existing graphics code becomes part of `engine.rendering`
- Current math utilities integrate into `engine.platform`
- Pathfinding code becomes foundation for `colony.pathfinding`

## Success Metrics

### **Compilation Performance**

- **Build Time Reduction**: Target 50%+ reduction in full rebuild time
- **Incremental Build Speed**: Target 80%+ reduction in incremental builds
- **Parallel Compilation**: Achieve module-level parallelism

### **Code Quality**

- **Dependency Clarity**: Clear module import/export relationships
- **Interface Stability**: Implementation changes don't break importers
- **Testing Coverage**: Each module achieves 80%+ test coverage

### **Development Experience**

- **Hot-Reload Efficiency**: Module changes reload in <1 second
- **Debug Clarity**: Module boundaries simplify debugging scope
- **Team Productivity**: Parallel development without integration conflicts

This module architecture provides the foundation for implementing your complete engine design using modern C++20
modules, ensuring optimal compilation performance, clear dependency management, and excellent development workflow
integration.
