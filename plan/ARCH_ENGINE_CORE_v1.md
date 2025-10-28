# ARCH_ENGINE_CORE_v1

> **High-Level Architecture Document for Citrus Engine**

## Executive Summary

This document defines the foundational architecture for Citrus Engine, a modern, general-purpose 2D/3D game engine built with C++20,
designed for maximum flexibility while being developed primarily for colony simulation games. The engine emphasizes
clean ECS architecture, advanced multi-threading with frame pipelining, cross-platform compatibility (
Windows/Linux/WebAssembly), and a modular design that enables selective feature inclusion. The architecture prioritizes
performance through data-oriented design while maintaining developer productivity through comprehensive tooling and
scripting interfaces.

## Scope and Objectives

### In Scope

- [ ] Complete ECS architecture with data-oriented component design
- [ ] Multithreaded system execution with frame pipelining
- [ ] Cross-platform support: Windows, Linux, WebAssembly (Steam Deck compatible)
- [ ] 2D and 3D rendering with OpenGL backend (extensible to other APIs)
- [ ] Comprehensive input support: mouse, keyboard, controllers
- [ ] Asset pipeline supporting both filesystem and embedded loading
- [ ] Scripting interface for external language integration
- [ ] Audio system with OpenAL backend
- [ ] Development tools integration (hot-reload, debugging, profiling)

### Out of Scope

- [ ] Networking systems (future extension)
- [ ] Advanced rendering features requiring Vulkan/DirectX (Phase 2)
- [ ] Mobile platform support (future consideration)
- [ ] VR/AR support (future consideration)
- [ ] Built-in level editors (external tooling approach)

### Primary Objectives

1. **Performance**: Achieve 60+ FPS with 10,000+ entities through ECS and multi-threading
2. **Flexibility**: Enable rapid prototyping and diverse game types through modular architecture
3. **Cross-Platform**: Identical feature parity between native and WebAssembly builds
4. **Developer Experience**: Comprehensive tooling, debugging, and scripting capabilities

### Secondary Objectives

- Future-proof architecture supporting next-generation rendering APIs
- Plugin system enabling third-party extensions
- Comprehensive performance profiling and debugging tools
- Industry-standard asset pipeline compatibility

## Architecture/Design

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                        │
├─────────────────────────────────────────────────────────────────┤
│        Game Systems (AI, Pathfinding, Simulation, UI)           │
├─────────────────────────────────────────────────────────────────┤
│     Engine Systems (Rendering, Audio, Physics, Animation)       │
├─────────────────────────────────────────────────────────────────┤
│        Core Systems (ECS, Threading, Scene, Resources)          │
├─────────────────────────────────────────────────────────────────┤
│     Foundation (Platform, Math, Memory, Asset Pipeline)         │
└─────────────────────────────────────────────────────────────────┘

System Documentation Status:
Foundation Layer (8/8 documented): ✅ COMPLETE
- Platform Abstraction, ECS Core, Threading, Rendering, Audio, 
  Input, Assets, Scene Management

Engine Systems Layer (1/4 documented): 🔄 PARTIAL  
- Physics ✅ (SYS_PHYSICS_v1 - 2D collision, AI integration)
- Animation ❌ (not yet documented)
- Advanced Rendering Features ❌ (future phase)
- Scripting Interface ❌ (future phase)

Game Systems Layer (0/4+ documented): ❌ MISSING
- AI Decision Making ❌ (high priority)
- Pathfinding ❌ (high priority) 
- Colony Simulation ❌ (high priority)
- UI System ❌ (medium priority)

Threading Model (Current Implementation):
┌───────────────┬───────────────────────────────┬─────────────┐
│ Main Thread   │          Render Thread        │   I/O       │
│ (Events &     │         (Sequential)          │  Thread     │
│ Coordination) │                               │  Pool       │
│               │  ┌─────────────┐              │ (Assets)    │
│               │  │ System      │              │             │
│               │  │ Thread Pool │              │             │
│               │  │ (ECS Logic) │              │             │
│               │  └─────────────┘              │             │
│               │         ↓                     │             │
│               │  ┌─────────────┐              │             │
│               │  │   OpenGL    │              │             │
│               │  │ Rendering   │              │             │
│               │  └─────────────┘              │             │
└───────────────┴───────────────────────────────┴─────────────┘
      │                     │                         │
      │    ┌── Frame N ─────┤                         │
      └────┤ Main: Events   │                         │
           │ Render Thread: │←────────────────────────┘
           │ 1. ECS Systems │
           │ 2. GPU Render  │
           └────────────────┘
```

### Core Components

#### Component 1: ECS Foundation

- **Purpose**: Entity management, component storage, system scheduling
- **Responsibilities**: Entity lifecycle, component queries, system dependencies
- **Key Classes/Interfaces**: `World`, `EntityManager`, `ComponentManager`, `SystemScheduler`
- **Data Flow**: Systems query components → Process entities → Update components

#### Component 2: Threading Architecture

- **Purpose**: Parallel system execution, frame pipelining, work distribution
- **Responsibilities**: Job scheduling, thread-safe execution, memory ordering
- **Key Classes/Interfaces**: `JobSystem`, `ThreadPool`, `FramePipeline`, `WorkQueue`
- **Data Flow**: Main thread schedules → Worker threads execute → Synchronization points

#### Component 3: Rendering System

- **Purpose**: Graphics abstraction, scene rendering, cross-platform compatibility
- **Responsibilities**: Render command generation, GPU resource management, backend abstraction
- **Key Classes/Interfaces**: `Renderer`, `RenderBackend`, `RenderCommand`, `Scene`
- **Data Flow**: Scene data → Render commands → Backend execution → GPU

#### Component 4: Platform Abstraction

- **Purpose**: OS integration, input handling, window management
- **Responsibilities**: Platform-specific code isolation, input event processing
- **Key Classes/Interfaces**: `Platform`, `Window`, `InputManager`, `FileSystem`
- **Data Flow**: OS events → Platform layer → Engine systems

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Centralized entity manager with generation-based IDs
- **Entity Queries**: Template-based component queries with compile-time optimization
- **Component Dependencies**: Explicit system dependencies with automatic scheduling

#### Component Design

```cpp
// Data-only component structure
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    bool dirty = true;
};

// Component traits for ECS integration
template<>
struct ComponentTraits<TransformComponent> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 50000;
    static constexpr size_t alignment = alignof(glm::vec3);
};

// Render component with GPU data
struct RenderComponent {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t shader_id;
    bool visible = true;
    float depth_bias = 0.0f;
};
```

#### System Integration

- **System Dependencies**: Explicit declaration of read/write component access
- **Component Access Patterns**: Immutable views for read-only, exclusive access for writes
- **Inter-System Communication**: Event bus for loose coupling, shared components for tight coupling

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Logic (Update), Systems (Parallel), Render (Pipelined), I/O (Async)
- **Thread Safety**: Lock-free component access, atomic operations for shared state
- **Data Dependencies**: Automatic dependency resolution based on component access patterns

#### Parallel Execution Strategy

```cpp
class SystemScheduler {
public:
    // System can declare its threading requirements
    template<typename SystemType>
    void RegisterSystem(SystemType&& system) {
        auto requirements = system.GetThreadingRequirements();
        dependency_graph_.AddSystem(std::forward<SystemType>(system), requirements);
    }
    
    // Execute systems in parallel batches
    void ExecuteFrame(const FrameContext& context) {
        auto batches = dependency_graph_.GetExecutionBatches();
        for (const auto& batch : batches) {
            job_system_.SubmitBatch(batch, context);
            job_system_.WaitForCompletion();
        }
    }
};

// Example system with threading requirements
class PhysicsSystem : public ISystem {
public:
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements {
        return ThreadingRequirements{
            .read_components = {ComponentType::Transform, ComponentType::RigidBody},
            .write_components = {ComponentType::Transform, ComponentType::Velocity},
            .execution_phase = ExecutionPhase::Logic,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Component arrays with structure-of-arrays layout
- **Memory Ordering**: Sequential consistency for cross-system communication
- **Lock-Free Sections**: Component queries and iteration use lock-free data structures

### Current Threading Integration Strategy

#### Existing Threading Model Analysis

Based on the current implementation in `main.cpp` and `Game.cpp`, the engine already uses a sophisticated threading
approach:

```cpp
// Current main.cpp structure:
// Main Thread: Event handling via glfwWaitEvents()
// Render Thread: OpenGL context + game.Run() with rendering
std::jthread render_thread([&](const std::stop_token& stoken) {
    engine::os::OS::MakeContextCurrent(window);
    game.Run(stoken, window);
});
```

#### ECS Integration Points

The new ECS system will integrate into the existing `Game::Update()` method:

```cpp
// Current Game::Update() - where ECS systems will be inserted
void Game::Update(GLFWwindow* window, double delta_time) {
    if (engine_world) {
        engine_world->GetSystemScheduler().ExecuteFrame(delta_time);
    }
    
    // UI system must be updated, but should be done after ECS systems, as it can depend on ECS state
    ui_system.Update(window, render_system.GetViewport(), render_system.GetCamera());
}
```

#### Thread Synchronization Strategy

- **Main Thread**: Pure event handling via `glfwWaitEvents()`, no blocking operations
- **Render Thread**: Complete game loop execution including ECS system scheduling
- **System Thread Pool**: ECS systems execute in parallel within render thread context
- **OpenGL Context**: All rendering stays on render thread, avoiding context switching overhead
- **Synchronization**: ECS systems use lock-free component access with atomic operations for shared state

#### Current Implementation Benefits

Your existing threading model provides several advantages for ECS integration:

1. **Event Isolation**: Main thread purely handles input via `glfwWaitEvents()`, no ECS blocking concerns
2. **OpenGL Context Safety**: ECS systems run on render thread with direct OpenGL access for rendering components
3. **Clean Separation**: Game logic (including ECS) and rendering naturally isolated from input handling
4. **Performance**: Event-driven main thread is more efficient than polling, saves significant CPU cycles
5. **Deterministic**: All game state changes happen on single thread (render thread), simplifying synchronization

```

### Public APIs

#### Primary Interface: `Engine`

```cpp
#pragma once

#include <memory>
#include <optional>
#include <span>

namespace engine::core {

class Engine {
public:
    [[nodiscard]] auto Initialize(const EngineConfig& config) -> std::optional<std::string>;
    
    void RunFrame();
    void Shutdown() noexcept;
    
    // World management
    [[nodiscard]] auto CreateWorld() -> std::unique_ptr<World>;
    void SetActiveWorld(std::unique_ptr<World> world);
    [[nodiscard]] auto GetActiveWorld() -> World*;
    
    // System registration
    template<typename SystemType, typename... Args>
    void RegisterSystem(Args&&... args) {
        system_scheduler_.RegisterSystem<SystemType>(std::forward<Args>(args)...);
    }
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const EngineScriptInterface&;

private:
    bool is_initialized_ = false;
    EngineConfig config_;
    std::unique_ptr<World> active_world_;
    SystemScheduler system_scheduler_;
    std::unique_ptr<JobSystem> job_system_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Platform> platform_;
};

} // namespace engine::core
```

#### Secondary Interfaces

- `World` - Entity and component management within a game world
- `SystemScheduler` - System registration and parallel execution
- `Renderer` - Graphics abstraction with backend swapping capability
- `Platform` - OS abstraction for input, windowing, and file I/O

#### Scripting Interface Requirements

```cpp
// Engine scripting interface
class EngineScriptInterface {
public:
    // Entity management
    [[nodiscard]] auto CreateEntity() -> EntityId;
    void DestroyEntity(EntityId entity);
    
    // Component management
    template<typename ComponentType>
    void AddComponent(EntityId entity, const ComponentType& component);
    
    template<typename ComponentType>
    [[nodiscard]] auto GetComponent(EntityId entity) -> std::optional<ComponentType*>;
    
    // System control
    void SetSystemEnabled(std::string_view system_name, bool enabled);
    [[nodiscard]] auto GetSystemPerformanceMetrics(std::string_view system_name) -> PerformanceData;
    
    // Configuration management
    void LoadEngineConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetEngineConfiguration() const -> const ConfigData&;

private:
    Engine* engine_;
};
```

### Data Structures

#### Core Data Types

```cpp
// Entity identification
using EntityId = uint32_t;
constexpr EntityId INVALID_ENTITY = 0;

// Component identification
using ComponentTypeId = uint32_t;

// System execution phases
enum class ExecutionPhase : uint8_t {
    PreUpdate,
    Update,
    PostUpdate,
    PreRender,
    Render,
    PostRender
};

// Engine configuration
struct EngineConfig {
    std::string application_name;
    uint32_t window_width = 1920;
    uint32_t window_height = 1080;
    bool fullscreen = false;
    bool vsync = true;
    uint32_t max_entities = 100000;
    uint32_t worker_thread_count = 0; // 0 = hardware concurrency
    bool enable_debugging = false;
    std::filesystem::path asset_root_path;
};
```

#### Performance Considerations

- **Component Storage**: Structure-of-arrays for cache efficiency
- **Entity Queries**: Compile-time query optimization using C++20 concepts
- **Memory Pools**: Custom allocators for predictable allocation patterns
- **SIMD Opportunities**: Vectorized operations on component arrays

## Dependencies

### Internal Dependencies

- **Required Systems**: Platform abstraction must exist before all other systems
- **Optional Systems**: Audio, physics can be disabled for headless builds
- **Circular Dependencies**: Render system and scene management have bidirectional communication

### External Dependencies

- **Third-Party Libraries**:
    - `glm` for mathematics (header-only)
    - `glad` for OpenGL loading
    - `imgui` for development UI (already in vcpkg.json)
    - `openal-soft` for audio
    - `nlohmann-json` for configuration
- **Standard Library Features**: C++20 concepts, ranges, coroutines, modules
- **Platform APIs**: OpenGL 3.3+, platform-specific windowing and input

### Build System Dependencies

- **CMake Targets**: Each engine system as separate static library
- **vcpkg Packages**: All dependencies managed through vcpkg.json
- **Platform-Specific**: Emscripten for WebAssembly, native compilers for desktop
- **Module Dependencies**: C++20 modules for engine core, traditional headers for public API

### Asset Pipeline Dependencies

- **Asset Formats**: JSON for configuration, standard formats for textures/models
- **Configuration Files**: Follow existing patterns (items.json, structures.json)
- **Resource Loading**: Virtual filesystem for unified asset access

### Reference Implementation Examples

- **Existing Code Patterns**: Current `src/engine/math/` for mathematical foundations
- **Anti-Patterns**: Avoid deep inheritance hierarchies from existing graphics code
- **Integration Points**: Engine will provide higher-level interfaces than current `src/engine/`

### Dependency Graph

```
Platform ← Math ← Memory ← ECS ← Threading
    ↓         ↓       ↓       ↓        ↓
  Input → Scene → Assets → Systems → Renderer
    ↓         ↓       ↓         ↓        ↓
  UI ← Audio ← Physics ← Animation ← Materials
```

## Success Criteria

### Functional Requirements

- [ ] **ECS Core**: Create 100,000 entities with 10+ components each
- [ ] **Multi-Threading**: Execute 50+ systems in parallel with automatic dependency resolution
- [ ] **Cross-Platform**: Identical API on Windows, Linux, and WebAssembly
- [ ] **Rendering**: Support both 2D and 3D rendering with multiple render targets
- [ ] **Asset Pipeline**: Load assets from filesystem and embedded data sources
- [ ] **Scripting**: Expose all major engine functionality to external scripts

### Performance Requirements

- [ ] **Frame Rate**: Maintain 60 FPS with 10,000+ active entities
- [ ] **Memory**: Peak memory usage under 1GB for typical game scenarios
- [ ] **Startup Time**: Engine initialization under 100ms
- [ ] **Asset Loading**: Load typical game assets under 16ms per frame

### Quality Requirements

- [ ] **Reliability**: Zero crashes during 24-hour stress testing
- [ ] **Maintainability**: All public APIs documented with 90%+ coverage
- [ ] **Testability**: Unit test coverage above 80% for core systems
- [ ] **Documentation**: Complete API documentation and usage examples

### Acceptance Tests

```cpp
// Performance validation
TEST(EngineCore, HandlesLargeEntityCounts) {
    Engine engine;
    auto world = engine.CreateWorld();
    
    // Create 10,000 entities with transform and render components
    for (uint32_t i = 0; i < 10000; ++i) {
        auto entity = world->CreateEntity();
        world->AddComponent<TransformComponent>(entity, {});
        world->AddComponent<RenderComponent>(entity, {});
    }
    
    // Measure frame time over 1000 frames
    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t frame = 0; frame < 1000; ++frame) {
        engine.RunFrame();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto avg_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start) / 1000;
    EXPECT_LT(avg_frame_time.count(), 16666); // 60 FPS = 16.666ms per frame
}

// Cross-platform validation
TEST(EngineCore, IdenticalBehaviorAcrossPlatforms) {
    // Test that same inputs produce same outputs on all platforms
    EXPECT_TRUE(ValidateConsistentBehavior());
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Foundation (Estimated: 15 days)

- [ ] **Task 1.1**: Platform abstraction layer (3 days)
- [ ] **Task 1.2**: Basic ECS framework (5 days)
- [ ] **Task 1.3**: Simple threading infrastructure (main thread + thread pool) (4 days)
- [ ] **Task 1.4**: Asset pipeline foundation (3 days)
- [ ] **Deliverable**: Core engine that can create entities and run systems

#### Phase 2: Core Systems (Estimated: 28 days)

- [ ] **Task 2.1**: Rendering abstraction and OpenGL backend (7 days)
- [ ] **Task 2.2**: Scene management and transforms (4 days)
- [ ] **Task 2.3**: Input system with action mapping (3 days)
- [ ] **Task 2.4**: Resource management (5 days)
- [ ] **Task 2.5**: Audio system with OpenAL (4 days)
- [ ] **Task 2.6**: Basic physics/collision (5 days)
- [ ] **Deliverable**: Functional engine capable of rendering and interaction

#### Phase 3: Integration & Tools (Estimated: 12 days)

- [ ] **Task 3.1**: Scripting interface implementation (4 days)
- [ ] **Task 3.2**: Development tools integration (3 days)
- [ ] **Task 3.3**: Performance profiling and debugging (3 days)
- [ ] **Task 3.4**: WebAssembly build optimization (2 days)
- [ ] **Deliverable**: Production-ready engine with full toolchain

### File Structure

```
src/engine/
├── modules/
│   ├── engine.core.cppm          // Primary engine module interface
│   ├── engine.ecs.cppm           // ECS system module
│   ├── engine.rendering.cppm     // Rendering abstraction module
│   ├── engine.threading.cppm     // Threading and job system module
│   ├── engine.platform.cppm      // Platform abstraction module
│   ├── engine.assets.cppm        // Asset management module
│   └── engine.scripting.cppm     // Scripting interface module
├── impl/
│   ├── core/
│   │   ├── Engine.cpp
│   │   ├── World.cpp
│   │   └── Types.h
│   ├── ecs/
│   │   ├── ComponentManager.cpp
│   │   ├── EntityManager.cpp
│   │   └── SystemScheduler.cpp
│   ├── threading/
│   │   ├── JobSystem.cpp
│   │   ├── ThreadPool.cpp
│   │   └── WorkQueue.cpp
│   ├── platform/
│   │   ├── Platform.cpp
│   │   ├── Window.cpp
│   │   └── FileSystem.cpp
│   ├── rendering/
│   │   ├── Renderer.cpp
│   │   ├── RenderBackend.cpp
│   │   └── opengl/
│   ├── assets/
│   │   ├── AssetManager.cpp
│   │   └── AssetLoader.cpp
│   └── scripting/
│       ├── ScriptInterface.cpp
│       └── ScriptBindings.cpp
└── tests/
    ├── ecs/
    ├── threading/
    ├── platform/
    └── rendering/
```

### Code Organization Patterns

- **Namespace**: `engine::core`, `engine::ecs`, `engine::rendering`, etc.
- **Header Guards**: Use `#pragma once` for internal implementation files only
- **Module Structure**: Public engine interfaces as C++20 modules, internal implementation as traditional
  headers/sources
- **Build Integration**: Each subsystem as separate CMake target with module interface units

### Testing Strategy

- **Unit Tests**: Component-level testing for all core systems
- **Integration Tests**: Cross-system interaction validation
- **Performance Tests**: Automated benchmarking with regression detection
- **Platform Tests**: Identical behavior validation across Windows/Linux/WASM

## Risk Assessment

### Technical Risks

| Risk                        | Probability | Impact | Mitigation                                   |
|-----------------------------|-------------|--------|----------------------------------------------|
| **WebAssembly Performance** | Medium      | High   | Optimize critical paths, profile early       |
| **ECS Complexity**          | Low         | High   | Incremental development, extensive testing   |
| **Threading Bugs**          | Medium      | High   | Lock-free design, thorough testing           |
| **C++20 Compiler Support**  | Low         | Medium | Fallback implementations for older compilers |

### Integration Risks

- **Existing Codebase**: Risk of API incompatibilities with current colony game
- **Asset Pipeline**: Risk of breaking existing asset loading workflows
- **Performance Regression**: Risk of slower performance than current engine

### Resource Risks

- **Development Time**: ECS and threading complexity may extend timeline
- **WebAssembly Expertise**: Specialized knowledge required for optimization
- **Testing Complexity**: Multi-platform testing increases validation effort

### Contingency Plans

- **Plan B**: Simplified ECS implementation if full version proves too complex
- **Scope Reduction**: Audio and physics systems can be deferred to later phases
- **Timeline Adjustment**: Focus on core functionality first, polish features later

## Decision Rationale

### Architectural Decisions

#### Decision 1: ECS Architecture

- **Options Considered**: Traditional OOP, component-based, ECS, hybrid approaches
- **Selection Rationale**: ECS provides best performance and flexibility for complex simulations
- **Trade-offs**: Higher initial complexity for better long-term maintainability
- **Future Impact**: Enables data-driven design and optimal multi-threading

#### Decision 2: OpenGL First

- **Options Considered**: Vulkan, DirectX, OpenGL, abstracted backend
- **Selection Rationale**: WebAssembly support requires OpenGL, abstraction enables future upgrades
- **Trade-offs**: Limited to OpenGL features initially but maintains compatibility
- **Future Impact**: Backend abstraction allows later addition of Vulkan/DirectX

#### Decision 3: Frame Pipelining

- **Options Considered**: Single-threaded, parallel systems, frame pipelining
- **Selection Rationale**: Maximum CPU utilization while maintaining deterministic behavior
- **Trade-offs**: Added complexity for significant performance gains
- **Future Impact**: Scales naturally to higher core counts

### Third-Party Library Decisions

- **glm**: Industry standard, header-only, excellent SIMD support
- **glad**: Lightweight OpenGL loading, better than GLEW for our needs
- **OpenAL Soft**: Cross-platform audio with 3D positioning
- **ImGui**: Already integrated, excellent for development tools

## References

### Related Planning Documents

- `SYS_ECS_CORE_v1.md` - Detailed ECS system design (to be created)
- `SYS_RENDERING_ABSTRACTION_v1.md` - Rendering system architecture (to be created)
- `SYS_THREADING_JOBSYSTEM_v1.md` - Threading and job system design (to be created)

### External Resources

- [C++20 Concepts Tutorial](https://en.cppreference.com/w/cpp/language/concepts)
- [ECS Design Patterns](https://github.com/SanderMertens/ecs-faq)
- [WebAssembly Performance Guide](https://web.dev/webassembly-threads/)

### Existing Code References

- `src/engine/math/` - Foundation for new math system
- `src/graphics/RenderSystem.cpp` - Patterns to improve in new renderer
- `assets/*.json` - Asset structure patterns to maintain

## Appendices

### A. Detailed Performance Modeling

Target performance characteristics based on ECS design:

- Entity creation: < 1μs per entity
- Component queries: < 10μs for 10,000 entities
- System execution: < 100μs for typical game logic systems

### B. Cross-Platform Compatibility Matrix

| Feature   | Windows  | Linux    | WebAssembly |
|-----------|----------|----------|-------------|
| Threading | ✅ Full   | ✅ Full   | ⚠️ Limited  |
| File I/O  | ✅ Native | ✅ Native | ✅ Virtual   |
| Audio     | ✅ OpenAL | ✅ OpenAL | ✅ Web Audio |
| Input     | ✅ Full   | ✅ Full   | ✅ Limited   |

### C. Component Mapping Strategy

Since this is a clean-room implementation, the new ECS will coexist with current systems:

1. **Initial Development**: ECS systems developed independently of existing entity system
2. **Coexistence Phase**: Both systems run simultaneously during development and testing
3. **Performance Validation**: Direct comparison of ECS vs. current system performance
4. **Selective Adoption**: Individual game features can choose ECS or existing systems based on needs

---

**Document Status**: Draft
**Last Updated**: June 30, 2025
**Next Review**: July 7, 2025
**Reviewers**: [To be assigned]
