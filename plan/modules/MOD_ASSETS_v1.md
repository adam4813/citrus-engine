# MOD_ASSETS_v1

> **Asset Loading and Hot-Reload System - Foundation Module**

## Executive Summary

The `engine.assets` module provides a comprehensive asset management system with asynchronous loading, hot-reload
capabilities, memory management, and cross-platform file format support serving as the foundation for all game content
loading including textures, models, audio, scripts, and configuration files. This module implements efficient caching,
streaming, and bundling systems that enable seamless content delivery across desktop and WebAssembly platforms while
supporting real-time development workflows through hot-reload functionality for the Colony Game Engine's extensive asset
requirements.

## Scope and Objectives

### In Scope

- [ ] Asynchronous asset loading with priority queuing
- [ ] Hot-reload system with file watching and dependency tracking
- [ ] Cross-platform asset streaming (filesystem and embedded)
- [ ] Memory-efficient caching with LRU eviction
- [ ] Asset bundling and compression for distribution
- [ ] Type-safe asset handles with reference counting

### Out of Scope

- [ ] Asset authoring tools and editors
- [ ] Advanced compression algorithms (handled by platform libraries)
- [ ] Network-based asset delivery (handled by engine.networking)
- [ ] Real-time asset generation

### Primary Objectives

1. **Loading Performance**: Load critical assets under 100ms on typical hardware
2. **Memory Efficiency**: Maintain asset memory usage under 512MB for full game content
3. **Hot-Reload Speed**: Detect and reload changed assets within 200ms during development

### Secondary Objectives

- Zero-copy asset loading for large files where possible
- 95%+ cache hit rate for frequently accessed assets
- Seamless transition between development and production asset pipelines

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages asset reference components for entities requiring resources
- **Entity Queries**: Queries entities with AssetReference, MaterialAsset, or TextureAsset components
- **Component Dependencies**: Provides asset data for rendering, audio, and physics components

#### Component Design

```cpp
// Asset-related components for ECS integration
struct AssetReference {
    AssetId asset_id{0};
    AssetType type{AssetType::Unknown};
    LoadState state{LoadState::Unloaded};
    std::weak_ptr<Asset> cached_asset;
};

struct TextureAsset {
    AssetId texture_id{0};
    Vec2 dimensions{0.0f, 0.0f};
    TextureFormat format{TextureFormat::RGBA8};
    std::uint32_t mip_levels{1};
    bool is_compressed{false};
};

struct MaterialAsset {
    AssetId material_id{0};
    std::vector<AssetId> texture_dependencies;
    AssetId shader_id{0};
    std::unordered_map<std::string, UniformValue> uniforms;
};

// Component traits for asset management
template<>
struct ComponentTraits<AssetReference> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 100000;
};

template<>
struct ComponentTraits<TextureAsset> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 10000;
};
```

#### System Integration

- **System Dependencies**: Loads before rendering, audio, and physics systems need assets
- **Component Access Patterns**: Read-write access to AssetReference states; read-only for asset data
- **Inter-System Communication**: Provides asset loaded/unloaded events for dependent systems

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Foundation loading - executes during initialization and asynchronously during runtime
- **Thread Safety**: All asset operations are thread-safe with lock-free reference counting
- **Data Dependencies**: No dependencies on other systems; provides data for all systems

#### Parallel Execution Strategy

```cpp
// Multi-threaded asset loading with dependency resolution
class AssetSystem : public ISystem {
public:
    // Parallel asset loading with priority scheduling
    void LoadAssetsAsync(std::span<AssetId> asset_ids, 
                        LoadPriority priority,
                        const ThreadContext& context) override;
    
    // Background processing of asset operations
    void ProcessAssetQueue(const ThreadContext& context);
    
    // Hot-reload monitoring on separate thread
    void MonitorFileChanges(const ThreadContext& context);

private:
    struct LoadRequest {
        AssetId asset_id;
        LoadPriority priority;
        std::vector<AssetId> dependencies;
        std::promise<AssetHandle> completion;
    };
    
    // Lock-free queues for asset operations
    moodycamel::ConcurrentQueue<LoadRequest> load_queue_;
    moodycamel::ConcurrentQueue<AssetId> reload_queue_;
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Asset data stored in contiguous pools for optimal cache usage
- **Memory Ordering**: Asset reference updates use acquire-release semantics
- **Lock-Free Sections**: Asset loading, caching, and reference counting are lock-free

### Public APIs

#### Primary Interface: `AssetManagerInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <future>

namespace engine::assets {

template<typename T>
concept AssetData = requires(T t) {
    typename T::AssetType;
    { t.GetAssetId() } -> std::convertible_to<AssetId>;
    { t.IsValid() } -> std::convertible_to<bool>;
};

class AssetManagerInterface {
public:
    [[nodiscard]] auto Initialize(const AssetConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Synchronous asset loading
    template<AssetData T>
    [[nodiscard]] auto LoadAsset(const AssetPath& path) -> std::optional<AssetHandle<T>>;
    
    // Asynchronous asset loading
    template<AssetData T>
    [[nodiscard]] auto LoadAssetAsync(const AssetPath& path, 
                                     LoadPriority priority = LoadPriority::Normal) -> std::future<AssetHandle<T>>;
    
    // Asset queries and management
    [[nodiscard]] auto IsAssetLoaded(AssetId id) const noexcept -> bool;
    [[nodiscard]] auto GetAssetState(AssetId id) const noexcept -> LoadState;
    void UnloadAsset(AssetId id) noexcept;
    
    // Hot-reload functionality
    void EnableHotReload(bool enable) noexcept;
    [[nodiscard]] auto RegisterReloadCallback(AssetId id, 
                                             std::function<void(AssetHandle<>)> callback) -> CallbackId;
    
    // Memory management
    void SetMemoryBudget(std::size_t bytes) noexcept;
    [[nodiscard]] auto GetMemoryUsage() const noexcept -> AssetMemoryInfo;
    void GarbageCollect() noexcept;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<AssetManagerImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::assets
```

#### Scripting Interface Requirements

```cpp
// Asset scripting interface for dynamic content loading
class AssetScriptInterface {
public:
    // Type-safe asset loading from scripts
    [[nodiscard]] auto LoadTexture(std::string_view path) -> std::optional<AssetId>;
    [[nodiscard]] auto LoadAudio(std::string_view path) -> std::optional<AssetId>;
    [[nodiscard]] auto LoadConfig(std::string_view path) -> std::optional<std::string>;
    
    // Asset state queries
    [[nodiscard]] auto IsAssetLoaded(AssetId id) const -> bool;
    [[nodiscard]] auto GetAssetPath(AssetId id) const -> std::optional<std::string>;
    
    // Hot-reload event registration
    [[nodiscard]] auto OnAssetReloaded(AssetId id, std::string_view callback_function) -> bool;
    
    // Memory monitoring
    [[nodiscard]] auto GetAssetMemoryUsage() const -> std::size_t;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<AssetManagerInterface> asset_manager_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Cross-Platform Loading**: Load all supported asset types on Windows, Linux, and WebAssembly
- [ ] **Hot-Reload**: Detect and reload changed assets within 200ms during development
- [ ] **Memory Management**: Automatically unload unused assets when memory budget exceeded

### Performance Requirements

- [ ] **Loading Speed**: Load critical assets (textures, configs) under 100ms
- [ ] **Memory Usage**: Maintain total asset memory under 512MB for full game content
- [ ] **Cache Efficiency**: Achieve 95%+ cache hit rate for frequently accessed assets
- [ ] **Throughput**: Process 1000+ asset requests per second during bulk loading

### Quality Requirements

- [ ] **Reliability**: Zero asset corruption during hot-reload operations
- [ ] **Maintainability**: All asset code covered by unit tests with mock file systems
- [ ] **Testability**: Support headless asset loading for automated testing
- [ ] **Documentation**: Complete asset pipeline documentation with format specifications

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(AssetsTest, LoadingPerformance) {
    auto asset_manager = engine::assets::AssetManager{};
    asset_manager.Initialize(AssetConfig{});
    
    // Test critical asset loading speed
    auto start = std::chrono::high_resolution_clock::now();
    auto texture_handle = asset_manager.LoadAsset<TextureAsset>("test_texture.png");
    auto end = std::chrono::high_resolution_clock::now();
    
    auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(load_time, 100); // Under 100ms
    EXPECT_TRUE(texture_handle.has_value());
    EXPECT_TRUE(texture_handle->IsValid());
}

TEST(AssetsTest, HotReloadSpeed) {
    auto asset_manager = engine::assets::AssetManager{};
    asset_manager.Initialize(AssetConfig{});
    asset_manager.EnableHotReload(true);
    
    // Load initial asset
    auto handle = asset_manager.LoadAsset<ConfigAsset>("test_config.json");
    ASSERT_TRUE(handle.has_value());
    
    auto initial_data = handle->GetData();
    
    // Modify file and measure reload time
    ModifyTestFile("test_config.json");
    
    auto start = std::chrono::high_resolution_clock::now();
    WaitForReload(handle->GetAssetId());
    auto end = std::chrono::high_resolution_clock::now();
    
    auto reload_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(reload_time, 200); // Under 200ms
    EXPECT_NE(handle->GetData(), initial_data); // Data changed
}

TEST(AssetsTest, MemoryManagement) {
    auto asset_manager = engine::assets::AssetManager{};
    asset_manager.Initialize(AssetConfig{});
    asset_manager.SetMemoryBudget(100 * 1024 * 1024); // 100MB budget
    
    // Load assets until budget exceeded
    std::vector<AssetHandle<TextureAsset>> handles;
    for (int i = 0; i < 1000; ++i) {
        auto handle = asset_manager.LoadAsset<TextureAsset>(
            fmt::format("test_texture_{}.png", i));
        if (handle) {
            handles.push_back(*handle);
        }
    }
    
    auto memory_info = asset_manager.GetMemoryUsage();
    EXPECT_LE(memory_info.total_used, 100 * 1024 * 1024); // Within budget
    EXPECT_GT(memory_info.evicted_count, 0); // Some assets evicted
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Asset System (Estimated: 6 days)

- [ ] **Task 1.1**: Implement basic asset registry and handle system
- [ ] **Task 1.2**: Add synchronous asset loading with type safety
- [ ] **Task 1.3**: Implement memory management with LRU caching
- [ ] **Deliverable**: Basic asset loading and caching working

#### Phase 2: Asynchronous Loading (Estimated: 4 days)

- [ ] **Task 2.1**: Implement async loading with priority queues
- [ ] **Task 2.2**: Add dependency resolution for composite assets
- [ ] **Task 2.3**: Integrate with threading system for parallel loading
- [ ] **Deliverable**: Non-blocking asset loading operational

#### Phase 3: Hot-Reload System (Estimated: 5 days)

- [ ] **Task 3.1**: Implement file watching across platforms
- [ ] **Task 3.2**: Add dependency tracking for reload propagation
- [ ] **Task 3.3**: Create reload callback system for dependent systems
- [ ] **Deliverable**: Real-time asset reloading during development

#### Phase 4: Asset Bundling (Estimated: 3 days)

- [ ] **Task 4.1**: Implement asset packing for distribution builds
- [ ] **Task 4.2**: Add compression and bundling tools
- [ ] **Task 4.3**: Support embedded assets for WebAssembly
- [ ] **Deliverable**: Production-ready asset distribution

### File Structure

```
src/engine/assets/
├── assets.cppm                // Primary module interface
├── asset_manager.cpp          // Core asset management
├── asset_registry.cpp         // Asset type registration and lookup
├── asset_loader.cpp           // File loading and parsing
├── asset_cache.cpp            // Memory management and caching
├── hot_reload_manager.cpp     // File watching and reload logic
├── asset_bundler.cpp          // Packaging for distribution
├── loaders/
│   ├── texture_loader.cpp     // Texture format support
│   ├── audio_loader.cpp       // Audio format support
│   ├── config_loader.cpp      // JSON/YAML configuration
│   └── shader_loader.cpp      // Shader compilation
└── tests/
    ├── asset_manager_tests.cpp
    ├── hot_reload_tests.cpp
    └── asset_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::assets`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with specialized loader submodules
- **Build Integration**: Links with engine.platform and engine.threading

### Testing Strategy

- **Unit Tests**: Mock file system for deterministic asset loading tests
- **Integration Tests**: Real file system testing with temporary directories
- **Performance Tests**: Loading speed and memory usage benchmarks
- **Hot-Reload Tests**: File modification simulation and callback verification

## Risk Assessment

### Technical Risks

| Risk                            | Probability | Impact | Mitigation                                       |
|---------------------------------|-------------|--------|--------------------------------------------------|
| **File System Race Conditions** | Medium      | High   | File locking, atomic operations for hot-reload   |
| **Memory Leaks in Caching**     | Low         | Medium | RAII patterns, automatic testing with Valgrind   |
| **Cross-Platform Path Issues**  | High        | Medium | Consistent path normalization, extensive testing |
| **Asset Corruption**            | Low         | High   | Checksums, validation on load, backup mechanisms |

### Integration Risks

- **ECS Performance**: Risk that asset component queries impact performance
    - *Mitigation*: Efficient component layout, cached asset references
- **Threading Deadlocks**: Risk of deadlocks between asset loading and file watching
    - *Mitigation*: Lock-free queues, careful lock ordering

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (file I/O, memory management, timing)
    - engine.threading (async loading, parallel processing)

- **Optional Systems**:
    - engine.profiling (loading performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 filesystem, futures, atomic operations
- **Third-Party Libraries**: JSON parsing (nlohmann/json), image loading (stb_image)

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.threading
- **vcpkg Packages**: nlohmann-json, stb (for image loading)
- **Platform-Specific**: Different file watching implementations per platform
- **Module Dependencies**: Imports engine.platform, engine.threading

### Asset Pipeline Dependencies

- **Asset Formats**: PNG/JPEG textures, OGG/WAV audio, JSON configurations
- **Configuration Files**: Follows existing JSON patterns (items.json, structures.json)
- **Resource Loading**: Both filesystem (development) and embedded (production) support
