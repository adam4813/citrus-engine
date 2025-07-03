# SYS_RENDERING_v1

> **System-Level Design Document for Cross-Platform Rendering System**

## Executive Summary

This document defines a high-performance, cross-platform rendering system for the modern C++20 game engine, designed to
efficiently render 10,000+ entities at 60+ FPS across native (Windows/Linux) and WebAssembly platforms. The system
abstracts OpenGL ES 2.0/WebGL differences through a command-based architecture that integrates seamlessly with the ECS
and threading model. The design emphasizes data-driven rendering, efficient batching, and clean separation between
high-level rendering logic and platform-specific graphics API calls.

## Scope and Objectives

### In Scope

- [ ] OpenGL ES 2.0/WebGL abstraction layer with identical feature parity
- [ ] Command-based rendering architecture for thread-safe operation
- [ ] ECS integration with efficient component queries and batching
- [ ] 2D and 3D rendering pipelines optimized for colony simulation
- [ ] Resource management for shaders, textures, buffers, and vertex arrays
- [ ] Debug rendering system for development tools and visualization
- [ ] Performance profiling hooks and GPU timing integration
- [ ] Multithreaded render command generation with single-threaded execution
- [ ] Declarative material and shader system using C++20 concepts

### Out of Scope

- [ ] Advanced rendering APIs (Vulkan, DirectX, Metal) - Phase 2 consideration
- [ ] Real-time ray tracing or advanced lighting models
- [ ] Post-processing effects requiring compute shaders
- [ ] Built-in level editor rendering (external tooling)
- [ ] VR/AR rendering pipelines
- [ ] Mobile-specific optimizations (texture compression, etc.)

### Primary Objectives

1. **Performance**: Render 10,000+ entities at 60+ FPS through efficient batching and culling
2. **Cross-Platform Parity**: Identical rendering behavior between native OpenGL and WebGL
3. **ECS Integration**: Seamless component-based rendering with minimal overhead
4. **Thread Safety**: Safe render command generation from multiple ECS systems
5. **WebGL Compliance**: Full compatibility with WebGL ES 2.0 subset limitations

### Secondary Objectives

- GPU resource streaming for large worlds without memory exhaustion
- Hot-reload support for shaders and materials during development
- Comprehensive debug visualization for ECS systems and engine internals
- Future extensibility for advanced rendering features and APIs
- Memory-efficient rendering for WebAssembly builds with limited heap

## Architecture/Design

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    ECS Rendering Systems                        │
├─────────────────────────────────────────────────────────────────┤
│  SpriteRenderSystem │ MeshRenderSystem │ DebugRenderSystem      │
│                     │                  │                        │
│  Query Components   │  Frustum Culling │  Line/Point Rendering  │
│  Generate Commands  │  LOD Selection   │  UI Debug Overlays     │
└─────────────────────┴──────────────────┴────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Render Command Buffer                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Draw Batch  │ │ State Change│ │ Debug Draw  │               │
│  │ Commands    │ │ Commands    │ │ Commands    │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Render Thread Executor                       │
├─────────────────────────────────────────────────────────────────┤
│  Command Sorting │ Batch Merging │ GPU Resource Management      │
│                  │               │                              │
│  State Tracking  │ Draw Call     │ Shader/Texture Binding      │
│                  │ Minimization  │                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              OpenGL ES 2.0 / WebGL Abstraction                  │
├─────────────────────────────────────────────────────────────────┤
│ Native OpenGL    │              WebGL Implementation            │
│ Implementation   │                                              │
│                  │  ┌─────────────┐ ┌─────────────┐           │
│ ┌─────────────┐  │  │ Buffer      │ │ Texture     │           │
│ │ Extensions  │  │  │ Orphaning   │ │ Format      │           │
│ │ & Features  │  │  │ Emulation   │ │ Validation  │           │
│ │ Detection   │  │  └─────────────┘ └─────────────┘           │
│ └─────────────┘  │                                              │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Render Command Architecture

- **Purpose**: Thread-safe render command generation and execution
- **Responsibilities**: Command buffering, sorting, batching, GPU state management
- **Key Classes/Interfaces**: `RenderCommandBuffer`, `RenderCommand`, `CommandExecutor`
- **Data Flow**: ECS Systems → Commands → Sorting → Execution → GPU

#### Component 2: Resource Management System

- **Purpose**: GPU resource lifecycle, streaming, and cross-platform compatibility
- **Responsibilities**: Shader compilation, texture loading, buffer management, memory tracking
- **Key Classes/Interfaces**: `ResourceManager`, `ShaderProgram`, `Texture2D`, `VertexBuffer`
- **Data Flow**: Asset Loading → Resource Creation → GPU Upload → Binding → Cleanup

#### Component 3: WebGL ES 2.0 Abstraction

- **Purpose**: Unified rendering API hiding platform differences
- **Responsibilities**: Feature detection, limitation handling, performance optimization
- **Key Classes/Interfaces**: `RenderDevice`, `GraphicsContext`, `RenderState`
- **Data Flow**: High-level API → Platform Detection → Native/WebGL Implementation

#### Component 4: ECS Rendering Integration

- **Purpose**: Efficient component queries and rendering data preparation
- **Responsibilities**: Frustum culling, LOD selection, batch generation, transform hierarchies
- **Key Classes/Interfaces**: `RenderSystem`, `CullingSystem`, `BatchGenerator`
- **Data Flow**: Component Queries → Culling → Batching → Command Generation

### WebGL ES 2.0 Constraint Handling

#### Forbidden Features and Workarounds

```cpp
// WebGL limitations and engine adaptations
namespace webgl_constraints {
    // No client-side arrays - all data must be in VBOs
    constexpr bool supports_client_arrays = false;
    
    // Limited texture formats
    constexpr std::array supported_formats = {
        TextureFormat::RGB8,
        TextureFormat::RGBA8,
        TextureFormat::LUMINANCE,
        TextureFormat::ALPHA
    };
    
    // No 3D textures, texture arrays, or cube map arrays
    constexpr bool supports_texture_3d = false;
    constexpr bool supports_texture_arrays = false;
    
    // Limited vertex attribute count
    constexpr size_t max_vertex_attributes = 8;
    
    // No uniform buffer objects
    constexpr bool supports_uniform_buffers = false;
}
```

#### Platform Abstraction Implementation

```cpp
// Cross-platform rendering device abstraction
class RenderDevice {
public:
    virtual ~RenderDevice() = default;
    
    // Pure virtual interface for platform-specific implementation
    [[nodiscard]] virtual auto CreateBuffer(BufferType type, size_t size, 
                                          const void* data = nullptr) -> std::unique_ptr<Buffer> = 0;
    
    [[nodiscard]] virtual auto CreateTexture(const TextureDesc& desc) -> std::unique_ptr<Texture> = 0;
    
    [[nodiscard]] virtual auto CreateShader(ShaderType type, 
                                           std::string_view source) -> std::unique_ptr<Shader> = 0;
    
    virtual void SetRenderState(const RenderState& state) = 0;
    virtual void Draw(const DrawCommand& command) = 0;
    
    // Capability queries for runtime adaptation
    [[nodiscard]] virtual auto GetCapabilities() const -> const DeviceCapabilities& = 0;
    [[nodiscard]] virtual auto SupportsExtension(std::string_view name) const -> bool = 0;
};

// WebGL-specific implementation
class WebGLRenderDevice final : public RenderDevice {
private:
    // WebGL context and capability tracking
    DeviceCapabilities capabilities_;
    
    // Emulation systems for missing features
    std::unique_ptr<BufferOrphaningEmulator> buffer_orphaning_;
    std::unique_ptr<UniformCaching> uniform_cache_;
    
public:
    auto CreateBuffer(BufferType type, size_t size, const void* data) 
        -> std::unique_ptr<Buffer> override;
    
    // WebGL-specific optimizations and workarounds
    void OptimizeForWebGL();
    void HandleContextLoss();
};
```

### ECS Integration Architecture

#### Rendering Components

```cpp
// Transform component for spatial data
struct TransformComponent {
    glm::mat4 world_matrix{1.0f};
    glm::mat4 local_matrix{1.0f};
    bool dirty = true;
    
    // Spatial hierarchy support
    std::optional<EntityID> parent;
    std::vector<EntityID> children;
};

// Renderable component for visual representation
struct RenderableComponent {
    ResourceID mesh_id;
    ResourceID material_id;
    uint32_t render_layer = 0;
    
    // LOD and culling data
    float bounding_radius = 1.0f;
    glm::vec3 bounding_center{0.0f};
    
    // Rendering flags
    bool cast_shadows = true;
    bool receive_shadows = true;
    bool visible = true;
};

// Sprite component for 2D rendering
struct SpriteComponent {
    ResourceID texture_id;
    glm::vec2 size{1.0f};
    glm::vec4 color{1.0f};
    glm::vec2 uv_offset{0.0f};
    glm::vec2 uv_scale{1.0f};
    uint32_t layer = 0;
};
```

#### Rendering Systems

```cpp
// Main rendering system coordinating all render operations
class RenderSystem final : public System {
public:
    void Update(World& world, float delta_time) override;
    
private:
    void ProcessTransforms(World& world);
    void ProcessRenderables(World& world);
    void ProcessSprites(World& world);
    void SubmitRenderCommands();
    
    RenderCommandBuffer command_buffer_;
    CullingSystem culling_system_;
    BatchGenerator batch_generator_;
};

// Efficient culling system using spatial queries
class CullingSystem {
public:
    auto CullEntities(const Camera& camera, 
                     const std::vector<EntityID>& entities,
                     const ComponentManager& components) 
        -> std::vector<EntityID>;
    
private:
    // Frustum culling with bounding volumes
    auto FrustumCull(const Frustum& frustum, 
                    const TransformComponent& transform,
                    const RenderableComponent& renderable) -> bool;
    
    // LOD selection based on distance
    auto SelectLOD(float distance, const RenderableComponent& renderable) -> uint32_t;
};
```

### Threading Integration

#### Render Command Generation

```cpp
// Thread-safe command buffer for multi-system rendering
class RenderCommandBuffer {
public:
    // Thread-safe command submission from ECS systems
    void SubmitCommand(RenderCommand command);
    void SubmitBatch(std::span<const RenderCommand> commands);
    
    // Main thread: prepare commands for render thread
    auto PrepareForExecution() -> ExecutionBatch;
    
    // Render thread: execute commands on GPU
    void ExecuteCommands(const ExecutionBatch& batch, RenderDevice& device);
    
private:
    // Lock-free command submission using atomics
    std::atomic<size_t> write_index_{0};
    std::array<RenderCommand, 65536> command_ring_buffer_;
    
    // Command sorting and batching for optimal GPU performance
    void SortCommands(std::span<RenderCommand> commands);
    void GenerateBatches(std::span<const RenderCommand> commands);
};
```

#### Frame Pipelining Support

```cpp
// Integration with engine's frame pipelining architecture
class RenderThreadCoordinator {
public:
    // Called from main thread after ECS systems complete
    void SubmitFrame(std::unique_ptr<RenderCommandBuffer> commands,
                    const CameraData& camera_data);
    
    // Render thread execution
    void ExecuteFrame(RenderDevice& device);
    
    // Synchronization with main thread
    void WaitForFrameCompletion();
    [[nodiscard]] auto IsFrameReady() const -> bool;
    
private:
    // Double-buffered command submission
    std::array<std::unique_ptr<RenderCommandBuffer>, 2> frame_buffers_;
    std::atomic<size_t> current_frame_{0};
    
    // Thread synchronization
    std::mutex frame_mutex_;
    std::condition_variable frame_ready_;
};
```

### Performance Optimization Strategies

#### Batching and State Minimization

```cpp
// Intelligent batching system for draw call reduction
class BatchGenerator {
public:
    auto GenerateBatches(std::span<const RenderCommand> commands) 
        -> std::vector<DrawBatch>;
    
private:
    // Batch compatibility checking
    auto CanBatch(const RenderCommand& a, const RenderCommand& b) const -> bool;
    
    // State change cost analysis
    auto CalculateStateCost(const RenderState& from, const RenderState& to) const -> uint32_t;
    
    // Optimal sorting for GPU pipeline efficiency
    void SortForBatching(std::span<RenderCommand> commands);
};

// GPU state tracking to minimize redundant calls
class RenderStateCache {
public:
    void BindShader(ResourceID shader_id);
    void BindTexture(uint32_t unit, ResourceID texture_id);
    void SetBlendState(const BlendState& state);
    void SetDepthState(const DepthState& state);
    
private:
    // Current GPU state tracking
    ResourceID current_shader_{ResourceID::Invalid};
    std::array<ResourceID, 16> bound_textures_;
    BlendState current_blend_state_;
    DepthState current_depth_state_;
    
    // Dirty state flags for selective updates
    std::bitset<32> dirty_flags_;
};
```

## Implementation Details

### Resource Management

#### Shader System with C++20 Concepts

```cpp
// Type-safe shader parameter concepts
template<typename T>
concept UniformType = std::same_as<T, float> || 
                     std::same_as<T, glm::vec2> ||
                     std::same_as<T, glm::vec3> ||
                     std::same_as<T, glm::vec4> ||
                     std::same_as<T, glm::mat4>;

// Declarative shader program definition
class ShaderProgram {
public:
    template<UniformType T>
    void SetUniform(std::string_view name, const T& value);
    
    void Bind();
    void Unbind();
    
    [[nodiscard]] auto IsValid() const -> bool;
    [[nodiscard]] auto GetUniformLocation(std::string_view name) const -> int32_t;
    
private:
    uint32_t program_id_ = 0;
    std::unordered_map<std::string, int32_t> uniform_locations_;
    
    // Platform-specific compilation
    auto CompileShader(ShaderType type, std::string_view source) -> uint32_t;
    auto LinkProgram(uint32_t vertex_shader, uint32_t fragment_shader) -> uint32_t;
};
```

#### Texture Management

```cpp
// Cross-platform texture abstraction
class Texture2D {
public:
    Texture2D(uint32_t width, uint32_t height, TextureFormat format);
    ~Texture2D();
    
    void Upload(const void* data, size_t size);
    void Bind(uint32_t unit);
    
    [[nodiscard]] auto GetWidth() const -> uint32_t { return width_; }
    [[nodiscard]] auto GetHeight() const -> uint32_t { return height_; }
    [[nodiscard]] auto GetFormat() const -> TextureFormat { return format_; }
    
private:
    uint32_t texture_id_ = 0;
    uint32_t width_, height_;
    TextureFormat format_;
    
    // WebGL-specific format validation
    void ValidateFormatSupport();
};
```

### Debug Rendering System

```cpp
// Development and debugging visualization
class DebugRenderer {
public:
    // Immediate mode debug rendering
    void DrawLine(const glm::vec3& start, const glm::vec3& end, 
                 const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    
    void DrawBox(const glm::vec3& center, const glm::vec3& size,
                const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    
    void DrawSphere(const glm::vec3& center, float radius,
                   const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    
    // Text rendering for debug overlays
    void DrawText(const glm::vec2& position, std::string_view text,
                 const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    
    // ECS system integration
    void DrawEntityBounds(EntityID entity, const World& world);
    void DrawComponentInfo(EntityID entity, const World& world);
    
    // Frame management
    void BeginFrame();
    void EndFrame();
    void Render(const Camera& camera);
    
private:
    // Efficient line batch rendering
    std::vector<LineVertex> line_vertices_;
    std::unique_ptr<VertexBuffer> line_vbo_;
    std::unique_ptr<ShaderProgram> line_shader_;
    
    // Text rendering resources
    std::unique_ptr<BitmapFont> debug_font_;
    std::vector<TextVertex> text_vertices_;
};
```

## Success Criteria

### Performance Metrics

1. **Entity Rendering**: 10,000+ visible entities at 60+ FPS on target hardware
2. **Draw Call Efficiency**: <100 draw calls for typical colony scenes through batching
3. **Memory Usage**: <64MB GPU memory usage for typical gameplay scenarios
4. **Frame Time**: <16.67ms total frame time with <8ms spent in rendering

### Cross-Platform Compliance

1. **WebGL Compatibility**: 100% feature parity between native and WebGL builds
2. **Visual Fidelity**: Identical rendering output across all supported platforms
3. **Performance Scaling**: WebGL performance within 30% of native performance
4. **Resource Loading**: Hot-reload support on native builds, efficient bundling for WebGL

### Development Experience

1. **Shader Hot-Reload**: <1 second shader recompilation and application
2. **Debug Visualization**: Comprehensive debug rendering for all engine systems
3. **Profiling Integration**: GPU timing and batch analysis tools
4. **Error Handling**: Clear error messages for shader compilation and resource loading failures

## Risk Mitigation

### WebGL Limitation Risks

- **Risk**: WebGL ES 2.0 constraints limiting rendering capabilities
- **Mitigation**: Comprehensive abstraction layer with feature emulation where possible
- **Fallback**: Graceful degradation for unsupported features

### Performance Risks

- **Risk**: Batching complexity reducing development velocity
- **Mitigation**: Incremental batching implementation with performance monitoring
- **Fallback**: Individual draw calls with state caching for worst-case scenarios

### Threading Complexity Risks

- **Risk**: Multi-threaded rendering introducing synchronization bugs
- **Mitigation**: Lock-free command buffer design with comprehensive testing
- **Fallback**: Single-threaded fallback mode for debugging and WebAssembly

This rendering system design provides a robust foundation for the engine's visual capabilities while respecting WebGL
constraints and integrating seamlessly with the established ECS and threading architecture.
