# MOD_RENDERING_v1

> **Graphics Abstraction and Rendering Pipeline - Foundation Module**

## Executive Summary

The `engine.rendering` module provides a modern, cross-platform graphics abstraction layer built on OpenGL ES 2.0/WebGL for maximum compatibility across desktop and web platforms. This module implements a flexible rendering pipeline with support for 2D/3D graphics, efficient batching, material systems, and extensible shader management, enabling high-performance visual rendering for the Colony Game Engine's isometric tile-based world with thousands of dynamic entities and environmental effects.

## Scope and Objectives

### In Scope

- [ ] Cross-platform graphics API abstraction (OpenGL ES 2.0/WebGL)
- [ ] Efficient 2D/3D rendering pipeline with batching
- [ ] Material system with shader management and hot-reload
- [ ] Camera system with multiple projection modes
- [ ] Texture management with compression and streaming
- [ ] Render state management and optimization

### Out of Scope

- [ ] Advanced 3D features (shadows, post-processing, PBR)
- [ ] Platform-specific optimizations (Vulkan, DirectX)
- [ ] Video playback and advanced particle systems
- [ ] VR/AR rendering support

### Primary Objectives

1. **High Performance**: Render 10,000+ sprites at 60 FPS on mid-range hardware
2. **Cross-Platform Compatibility**: Identical rendering output on Windows/Linux/WebAssembly
3. **Developer Efficiency**: Hot-reload shaders and materials without restart

### Secondary Objectives

- GPU memory usage under 256MB for typical scenes
- Draw call batching achieving 90%+ efficiency
- Sub-frame latency for real-time rendering

## Architecture/Design

### ECS Architecture Design

#### Entity Management
- **Entity Creation/Destruction**: Manages render components for all visible entities
- **Entity Queries**: Queries entities with Transform, Sprite, Mesh, or Material components
- **Component Dependencies**: Requires Transform for positioning; optional Material for custom rendering

#### Component Design
```cpp
// Core rendering components for ECS integration
struct Transform {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles
    Vec3 scale{1.0f, 1.0f, 1.0f};
    Mat4 world_matrix; // Cached transformation matrix
};

struct Sprite {
    TextureId texture{0};
    Vec2 size{1.0f, 1.0f};
    Vec2 texture_offset{0.0f, 0.0f};
    Vec2 texture_scale{1.0f, 1.0f};
    Color tint{1.0f, 1.0f, 1.0f, 1.0f};
    std::int32_t layer{0};
};

struct Material {
    ShaderId shader{0};
    std::vector<TextureBinding> textures;
    std::vector<UniformValue> uniforms;
    BlendMode blend_mode{BlendMode::Alpha};
    bool depth_test{true};
};

// Component traits for rendering
template<>
struct ComponentTraits<Transform> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 50000;
};

template<>
struct ComponentTraits<Sprite> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 20000;
};
```

#### System Integration
- **System Dependencies**: Runs after logic updates, before audio/UI systems
- **Component Access Patterns**: Read-only access to Transform/Sprite; read-write for cached matrices
- **Inter-System Communication**: Provides visibility queries for physics and AI systems

### Multi-Threading Design

#### Threading Model
- **Execution Phase**: Render batch - executes on main thread with GPU command preparation
- **Thread Safety**: Command buffer generation is thread-safe; OpenGL calls on main thread only
- **Data Dependencies**: Reads Transform/Sprite data; writes to GPU command buffers

#### Parallel Execution Strategy
```cpp
// Multi-threaded rendering preparation
class RenderSystem : public ISystem {
public:
    // Parallel culling and command generation
    void PrepareRenderCommands(const ComponentManager& components,
                              std::span<EntityId> entities,
                              const ThreadContext& context) override;
    
    // Main thread GPU submission
    void SubmitToGPU(const RenderCommandBuffer& commands);
    
    // Parallel batch sorting for depth/material optimization
    void SortRenderBatches(std::span<RenderBatch> batches, 
                          const ThreadContext& context);

private:
    struct RenderCommand {
        Mat4 transform;
        MaterialId material;
        MeshId mesh;
        std::uint32_t sort_key;
    };
};
```

#### Memory Access Patterns
- **Cache Efficiency**: Component data laid out for optimal transform matrix calculation
- **Memory Ordering**: GPU resource updates use release semantics for synchronization
- **Lock-Free Sections**: Command buffer generation and batch sorting are lock-free

### Public APIs

#### Primary Interface: `RenderSystemInterface`
```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>

namespace engine::rendering {

template<typename T>
concept RenderableComponent = requires(T t) {
    requires std::is_trivially_copyable_v<T>;
    { t.GetBoundingBox() } -> std::convertible_to<BoundingBox>;
};

class RenderSystemInterface {
public:
    [[nodiscard]] auto Initialize(const RenderConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Resource management
    [[nodiscard]] auto CreateTexture(const TextureDesc& desc) -> TextureId;
    [[nodiscard]] auto CreateShader(std::string_view vertex_source, 
                                   std::string_view fragment_source) -> ShaderId;
    [[nodiscard]] auto CreateMaterial(const MaterialDesc& desc) -> MaterialId;
    
    // Rendering operations
    void BeginFrame(const Camera& camera);
    void RenderBatch(std::span<const RenderCommand> commands);
    void EndFrame();
    
    // Camera management
    void SetActiveCamera(const Camera& camera);
    [[nodiscard]] auto GetActiveCamera() const -> const Camera&;
    
    // Performance queries
    [[nodiscard]] auto GetRenderStats() const -> RenderStatistics;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<RenderSystemImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::rendering
```

#### Scripting Interface Requirements
```cpp
// Rendering scripting interface for visual effects and debugging
class RenderingScriptInterface {
public:
    // Type-safe material and texture access
    [[nodiscard]] auto LoadTexture(std::string_view path) -> std::optional<TextureId>;
    [[nodiscard]] auto CreateMaterial(std::string_view shader_name) -> std::optional<MaterialId>;
    
    // Visual effect control
    void SetMaterialUniform(MaterialId material, std::string_view name, float value);
    void SetMaterialTexture(MaterialId material, std::string_view name, TextureId texture);
    
    // Camera control for cutscenes/debugging
    void SetCameraPosition(const Vec3& position);
    void SetCameraTarget(const Vec3& target);
    
    // Performance monitoring
    [[nodiscard]] auto GetDrawCallCount() const -> std::uint32_t;
    [[nodiscard]] auto GetGPUMemoryUsage() const -> std::size_t;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<RenderSystemInterface> render_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Cross-Platform Rendering**: Identical visual output on Windows, Linux, and WebAssembly
- [ ] **Sprite Rendering**: Efficiently render 10,000+ sprites with batching
- [ ] **Shader Hot-Reload**: Reload shaders at runtime without application restart

### Performance Requirements

- [ ] **Frame Rate**: Maintain 60 FPS with 5,000 visible sprites on mid-range hardware
- [ ] **Draw Calls**: Batch rendering to under 100 draw calls for typical scenes
- [ ] **Memory**: GPU memory usage under 256MB for textures and buffers
- [ ] **Latency**: Sub-16ms frame latency for responsive rendering

### Quality Requirements

- [ ] **Reliability**: Zero graphics driver crashes during normal operation
- [ ] **Maintainability**: All rendering code covered by automated tests
- [ ] **Testability**: Headless rendering mode for automated testing
- [ ] **Documentation**: Complete shader authoring documentation

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(RenderingTest, SpriteRenderingPerformance) {
    auto render_system = engine::rendering::RenderSystem{};
    render_system.Initialize(RenderConfig{});
    
    // Create test scene with many sprites
    std::vector<RenderCommand> commands;
    for (int i = 0; i < 10000; ++i) {
        commands.push_back(CreateTestSprite(i));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    render_system.RenderBatch(commands);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(frame_time, 16); // Under 16ms for 60 FPS
}

TEST(RenderingTest, DrawCallBatching) {
    auto render_system = engine::rendering::RenderSystem{};
    render_system.Initialize(RenderConfig{});
    
    // Render scene with mixed materials
    std::vector<RenderCommand> commands = CreateMixedMaterialScene();
    render_system.RenderBatch(commands);
    
    auto stats = render_system.GetRenderStats();
    EXPECT_LT(stats.draw_calls, 100); // Efficient batching
    EXPECT_GT(stats.batching_efficiency, 0.9f); // >90% efficiency
}
```

## Dependencies

### Internal Dependencies

- **Required Systems**: 
  - engine.platform (window management, OpenGL context)
  - engine.ecs (component access, entity queries)
  - engine.assets (texture/shader loading)

- **Optional Systems**: 
  - engine.profiling (GPU timing, draw call tracking)

### External Dependencies

- **Standard Library Features**: C++20 ranges, concepts, span
- **Graphics APIs**: OpenGL ES 2.0/WebGL for cross-platform compatibility

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.assets
- **vcpkg Packages**: GLEW, GLFW (for OpenGL context management)
- **Platform-Specific**: OpenGL libraries, WebGL for WASM builds
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.assets

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Graphics Driver Issues** | Medium | High | Extensive driver testing, fallback rendering paths |
| **WebGL Limitations** | High | Medium | Feature detection, graceful degradation |
| **Memory Fragmentation** | Low | Medium | Pool allocators, texture atlasing |
| **Shader Compilation Errors** | Medium | Low | Shader validation, error reporting |

### Integration Risks

- **ECS Performance**: Risk that component queries impact rendering performance
  - *Mitigation*: Cached query results, efficient component layout
- **Asset Loading**: Risk of texture loading blocking rendering thread
  - *Mitigation*: Asynchronous loading, placeholder textures
