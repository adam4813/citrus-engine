# SYS_ASSETS_v1

> **System-Level Design Document for Asset Pipeline and Management**

## Executive Summary

This document defines a comprehensive asset pipeline system for the modern C++20 game engine, designed to provide
efficient loading, hot-reload capabilities, and cross-platform compatibility between native and WebAssembly builds. The
system emphasizes declarative asset configuration, dependency tracking, and seamless integration with the ECS
architecture and threading model. The design supports both development-time hot-reload workflows and
production-optimized asset bundling, while maintaining consistent behavior across all supported platforms.

## Scope and Objectives

### In Scope

- [ ] Unified asset loading abstraction for filesystem and embedded resources
- [ ] Hot-reload capabilities with automatic dependency tracking during development
- [ ] Cross-platform asset packaging (native filesystem vs WebAssembly embedded data)
- [ ] Declarative asset configuration using JSON with schema validation
- [ ] Integration with rendering system for GPU resource management
- [ ] Asset streaming and lazy loading for large worlds and memory-constrained environments
- [ ] Incremental asset building with change detection and optimization
- [ ] Thread-safe asset loading using the established I/O thread pool
- [ ] Asset dependency resolution and automatic rebuild triggering
- [ ] Development tools integration for asset browser and debugging
- [ ] Platform abstraction integration for file I/O and directory watching

### Out of Scope

- [ ] Complex asset authoring tools (external content creation pipeline)
- [ ] Real-time asset compression and decompression (use preprocessed assets)
- [ ] Network-based asset distribution (future networking extension)
- [ ] Digital rights management or asset encryption
- [ ] Asset versioning and migration between major format changes
- [ ] Integration with external asset stores or content management systems
- [ ] Low-level file system operations (handled by Platform Abstraction system)

### Primary Objectives

1. **Developer Experience**: <1 second hot-reload for typical game assets during development
2. **Cross-Platform Compatibility**: Identical asset loading API between native and WebAssembly builds
3. **Performance**: Asset loading operations complete within frame budget (<16ms for typical assets)
4. **Memory Efficiency**: Intelligent asset streaming to maintain <512MB peak memory usage
5. **Reliability**: Robust error handling with graceful degradation for missing or corrupted assets

### Secondary Objectives

- Extensible asset type system supporting custom game-specific formats
- Integration with external asset processing tools (texture compression, audio encoding)
- Comprehensive asset usage analytics for optimization
- Support for modding through well-defined asset replacement mechanisms
- Future-proof design supporting asset format evolution

## Architecture/Design

### High-Level Overview

```
Asset Pipeline Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                   Asset Management Layer                        │
├─────────────────────────────────────────────────────────────────┤
│  AssetManager │ HotReloadWatcher │ DependencyTracker │ Cache    │
│               │                  │                   │          │
│  Asset        │ File System      │ Dependency Graph  │ LRU      │
│  Registry     │ Monitoring       │ Resolution        │ Eviction │
└─────────────────┴──────────────────┴───────────────────┴──────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Asset Loading Layer                         │
├─────────────────────────────────────────────────────────────────┤
│   Loader        │   Format        │   Resource        │ Stream  │
│   Factory       │   Handlers      │   Processors      │ Manager │
│                 │                 │                   │         │
│   JSON Config   │   Texture       │   GPU Upload      │ Async   │
│   Mesh Data     │   Audio         │   Optimization    │ I/O     │
│   Shader Code   │   Animation     │   Validation      │ Pool    │
└─────────────────┴─────────────────┴───────────────────┴─────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Platform Abstraction Layer                     │
├─────────────────────────────────────────────────────────────────┤
│    Native         │              WebAssembly                    │
│  Implementation   │             Implementation                  │
│                   │                                             │
│  ┌─────────────┐  │  ┌─────────────┐ ┌─────────────┐           │
│  │ File System │  │  │ Embedded    │ │ Browser     │           │
│  │ Access      │  │  │ Data        │ │ Assets      │           │
│  │             │  │  │ Extraction  │ │ (fetch API) │           │
│  │ Directory   │  │  │             │ │             │           │
│  │ Watching    │  │  │ Memory      │ │ IndexedDB   │           │
│  │             │  │  │ Mapping     │ │ Cache       │           │
│  └─────────────┘  │  └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Integration Points                          │
├─────────────────────────────────────────────────────────────────┤
│  ECS Components │ Rendering System │ Threading Model │ Config   │
│                 │                  │                 │          │
│  AssetComponent │ GPU Resource     │ I/O Thread Pool │ Engine   │
│  AssetReference │ Management       │ Integration     │ Config   │
│  AssetMetadata  │ Texture/Mesh     │ Job Scheduling  │ System   │
└─────────────────┴──────────────────┴─────────────────┴──────────┘
```

### Core Components

#### Component 1: Asset Management Core

- **Purpose**: Centralized asset registration, lifecycle management, and access coordination
- **Responsibilities**: Asset caching, reference counting, hot-reload coordination, dependency tracking
- **Key Classes/Interfaces**: `AssetManager`, `AssetRegistry`, `AssetHandle`, `AssetMetadata`
- **Data Flow**: Asset requests → Registry lookup → Loading/Caching → Handle distribution

#### Component 2: Cross-Platform Loading Abstraction

- **Purpose**: Unified asset loading API hiding platform implementation differences
- **Responsibilities**: Platform detection, loader selection, format validation, error handling
- **Key Classes/Interfaces**: `AssetLoader`, `PlatformAssetProvider`, `AssetSource`, `LoadResult`
- **Data Flow**: Load requests → Platform routing → Format processing → Resource creation

#### Component 3: Hot-Reload and Dependency System

- **Purpose**: Development-time asset monitoring and automatic reloading
- **Responsibilities**: File system watching, dependency graph maintenance, incremental updates
- **Key Classes/Interfaces**: `HotReloadWatcher`, `DependencyTracker`, `AssetDependency`
- **Data Flow**: File changes → Dependency analysis → Affected asset invalidation → Reload triggering

#### Component 4: Integration Bridges

- **Purpose**: Seamless integration with ECS, rendering, and threading systems
- **Responsibilities**: Component binding, GPU resource coordination, thread-safe access patterns
- **Key Classes/Interfaces**: `AssetComponent`, `GPUResourceManager`, `AssetJobScheduler`
- **Data Flow**: ECS queries → Asset resolution → GPU upload → Component updates

### Asset Management Core Implementation

#### Asset Handle and Reference System

```cpp
// Type-safe asset handle with automatic reference counting
template<typename AssetType>
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(AssetId id) : asset_id_(id) {
        if (asset_id_ != AssetId::Invalid) {
            AssetManager::Instance().AddReference(asset_id_);
        }
    }
    
    AssetHandle(const AssetHandle& other) : asset_id_(other.asset_id_) {
        if (asset_id_ != AssetId::Invalid) {
            AssetManager::Instance().AddReference(asset_id_);
        }
    }
    
    AssetHandle(AssetHandle&& other) noexcept : asset_id_(other.asset_id_) {
        other.asset_id_ = AssetId::Invalid;
    }
    
    ~AssetHandle() {
        if (asset_id_ != AssetId::Invalid) {
            AssetManager::Instance().RemoveReference(asset_id_);
        }
    }
    
    AssetHandle& operator=(const AssetHandle& other) {
        if (this != &other) {
            Reset();
            asset_id_ = other.asset_id_;
            if (asset_id_ != AssetId::Invalid) {
                AssetManager::Instance().AddReference(asset_id_);
            }
        }
        return *this;
    }
    
    AssetHandle& operator=(AssetHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            asset_id_ = other.asset_id_;
            other.asset_id_ = AssetId::Invalid;
        }
        return *this;
    }
    
    [[nodiscard]] auto Get() const -> const AssetType* {
        if (asset_id_ == AssetId::Invalid) {
            return nullptr;
        }
        return AssetManager::Instance().GetAsset<AssetType>(asset_id_);
    }
    
    [[nodiscard]] auto IsValid() const -> bool {
        return asset_id_ != AssetId::Invalid && Get() != nullptr;
    }
    
    [[nodiscard]] auto GetId() const -> AssetId { return asset_id_; }
    
    void Reset() {
        if (asset_id_ != AssetId::Invalid) {
            AssetManager::Instance().RemoveReference(asset_id_);
            asset_id_ = AssetId::Invalid;
        }
    }
    
private:
    AssetId asset_id_{AssetId::Invalid};
};

// Asset metadata for tracking and dependency management
struct AssetMetadata {
    AssetId id{AssetId::Invalid};
    std::string path;
    std::string type_name;
    size_t size_bytes = 0;
    std::chrono::file_time_type last_modified;
    std::vector<AssetId> dependencies;
    std::vector<AssetId> dependents;
    uint32_t reference_count = 0;
    bool is_loaded = false;
    bool hot_reload_enabled = true;
};
```

#### Central Asset Manager

```cpp
// Thread-safe asset management with hot-reload support
class AssetManager {
public:
    static auto Instance() -> AssetManager& {
        static AssetManager instance;
        return instance;
    }
    
    // Asset loading with type safety and caching
    template<typename AssetType>
    [[nodiscard]] auto LoadAsset(const std::string& path) -> AssetHandle<AssetType> {
        std::shared_lock lock(registry_mutex_);
        
        // Check if asset is already loaded
        if (auto it = path_to_id_.find(path); it != path_to_id_.end()) {
            return AssetHandle<AssetType>(it->second);
        }
        
        lock.unlock();
        std::unique_lock exclusive_lock(registry_mutex_);
        
        // Double-check after acquiring exclusive lock
        if (auto it = path_to_id_.find(path); it != path_to_id_.end()) {
            return AssetHandle<AssetType>(it->second);
        }
        
        // Create new asset entry
        const auto asset_id = GenerateAssetId();
        AssetMetadata metadata{
            .id = asset_id,
            .path = path,
            .type_name = typeid(AssetType).name(),
            .hot_reload_enabled = true
        };
        
        registry_[asset_id] = metadata;
        path_to_id_[path] = asset_id;
        
        // Schedule loading on I/O thread pool
        ScheduleAssetLoad<AssetType>(asset_id, path);
        
        return AssetHandle<AssetType>(asset_id);
    }
    
    // Immediate asset loading for synchronous requirements
    template<typename AssetType>
    [[nodiscard]] auto LoadAssetImmediate(const std::string& path) -> AssetHandle<AssetType> {
        auto handle = LoadAsset<AssetType>(path);
        WaitForAssetLoad(handle.GetId());
        return handle;
    }
    
    // Asset retrieval with type checking
    template<typename AssetType>
    [[nodiscard]] auto GetAsset(AssetId id) const -> const AssetType* {
        std::shared_lock lock(registry_mutex_);
        
        auto it = registry_.find(id);
        if (it == registry_.end() || !it->second.is_loaded) {
            return nullptr;
        }
        
        auto asset_it = loaded_assets_.find(id);
        if (asset_it == loaded_assets_.end()) {
            return nullptr;
        }
        
        // Type safety check
        if (it->second.type_name != typeid(AssetType).name()) {
            return nullptr;
        }
        
        return static_cast<const AssetType*>(asset_it->second.get());
    }
    
    // Reference counting for memory management
    void AddReference(AssetId id) {
        std::unique_lock lock(registry_mutex_);
        if (auto it = registry_.find(id); it != registry_.end()) {
            ++it->second.reference_count;
        }
    }
    
    void RemoveReference(AssetId id) {
        std::unique_lock lock(registry_mutex_);
        if (auto it = registry_.find(id); it != registry_.end()) {
            if (--it->second.reference_count == 0) {
                ScheduleAssetUnload(id);
            }
        }
    }
    
    // Hot-reload functionality
    void EnableHotReload(bool enable) {
        hot_reload_enabled_ = enable;
        if (enable && !hot_reload_watcher_) {
            hot_reload_watcher_ = std::make_unique<HotReloadWatcher>(*this);
        }
    }
    
    void OnAssetFileChanged(const std::string& path) {
        std::shared_lock lock(registry_mutex_);
        if (auto it = path_to_id_.find(path); it != path_to_id_.end()) {
            ScheduleAssetReload(it->second);
        }
    }
    
private:
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<AssetId, AssetMetadata> registry_;
    std::unordered_map<std::string, AssetId> path_to_id_;
    std::unordered_map<AssetId, std::unique_ptr<void, AssetDeleter>> loaded_assets_;
    
    std::atomic<uint32_t> next_asset_id_{1};
    bool hot_reload_enabled_ = false;
    std::unique_ptr<HotReloadWatcher> hot_reload_watcher_;
    
    // I/O thread pool integration
    std::unique_ptr<AssetJobScheduler> job_scheduler_;
    
    [[nodiscard]] auto GenerateAssetId() -> AssetId {
        return AssetId{next_asset_id_.fetch_add(1, std::memory_order_relaxed)};
    }
    
    template<typename AssetType>
    void ScheduleAssetLoad(AssetId id, const std::string& path);
    
    void ScheduleAssetUnload(AssetId id);
    void ScheduleAssetReload(AssetId id);
    void WaitForAssetLoad(AssetId id);
};
```

### Cross-Platform Loading Abstraction

#### Platform-Specific Asset Providers

```cpp
// Abstract interface for platform-specific asset access
class PlatformAssetProvider {
public:
    virtual ~PlatformAssetProvider() = default;
    
    [[nodiscard]] virtual auto LoadAssetData(const std::string& path) 
        -> std::expected<std::vector<uint8_t>, AssetError> = 0;
    
    [[nodiscard]] virtual auto GetAssetMetadata(const std::string& path) 
        -> std::expected<AssetMetadata, AssetError> = 0;
    
    [[nodiscard]] virtual auto ListAssets(const std::string& directory) 
        -> std::expected<std::vector<std::string>, AssetError> = 0;
    
    [[nodiscard]] virtual auto SupportsWatching() const -> bool = 0;
    virtual void StartWatching(const std::string& path, 
                              std::function<void(const std::string&)> callback) = 0;
};

// Native filesystem implementation
class FilesystemAssetProvider final : public PlatformAssetProvider {
public:
    explicit FilesystemAssetProvider(std::filesystem::path root_path) 
        : root_path_(std::move(root_path)) {}
    
    [[nodiscard]] auto LoadAssetData(const std::string& path) 
        -> std::expected<std::vector<uint8_t>, AssetError> override {
        
        const auto full_path = root_path_ / path;
        
        std::ifstream file(full_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return std::unexpected(AssetError::FileNotFound);
        }
        
        const auto size = file.tellg();
        file.seekg(0);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        if (!file) {
            return std::unexpected(AssetError::ReadFailed);
        }
        
        return data;
    }
    
    [[nodiscard]] auto GetAssetMetadata(const std::string& path) 
        -> std::expected<AssetMetadata, AssetError> override {
        
        const auto full_path = root_path_ / path;
        
        std::error_code ec;
        const auto last_write = std::filesystem::last_write_time(full_path, ec);
        if (ec) {
            return std::unexpected(AssetError::FileNotFound);
        }
        
        const auto size = std::filesystem::file_size(full_path, ec);
        if (ec) {
            return std::unexpected(AssetError::AccessFailed);
        }
        
        return AssetMetadata{
            .path = path,
            .size_bytes = size,
            .last_modified = last_write
        };
    }
    
    [[nodiscard]] auto SupportsWatching() const -> bool override { return true; }
    
    void StartWatching(const std::string& path, 
                      std::function<void(const std::string&)> callback) override {
        // Platform-specific directory watching implementation
        // Windows: ReadDirectoryChangesW, Linux: inotify
        file_watchers_[path] = std::make_unique<FileWatcher>(root_path_ / path, callback);
    }
    
private:
    std::filesystem::path root_path_;
    std::unordered_map<std::string, std::unique_ptr<FileWatcher>> file_watchers_;
};

// WebAssembly embedded data implementation
class EmbeddedAssetProvider final : public PlatformAssetProvider {
public:
    explicit EmbeddedAssetProvider(const uint8_t* embedded_data, size_t data_size) {
        ParseEmbeddedArchive(embedded_data, data_size);
    }
    
    [[nodiscard]] auto LoadAssetData(const std::string& path) 
        -> std::expected<std::vector<uint8_t>, AssetError> override {
        
        auto it = embedded_files_.find(path);
        if (it == embedded_files_.end()) {
            return std::unexpected(AssetError::FileNotFound);
        }
        
        const auto& file_info = it->second;
        std::vector<uint8_t> data(file_info.size);
        
        std::memcpy(data.data(), embedded_data_ + file_info.offset, file_info.size);
        
        return data;
    }
    
    [[nodiscard]] auto GetAssetMetadata(const std::string& path) 
        -> std::expected<AssetMetadata, AssetError> override {
        
        auto it = embedded_files_.find(path);
        if (it == embedded_files_.end()) {
            return std::unexpected(AssetError::FileNotFound);
        }
        
        return AssetMetadata{
            .path = path,
            .size_bytes = it->second.size,
            .last_modified = std::chrono::file_time_type{}
        };
    }
    
    [[nodiscard]] auto SupportsWatching() const -> bool override { return false; }
    
    void StartWatching(const std::string& path, 
                      std::function<void(const std::string&)> callback) override {
        // No-op for embedded assets - they cannot change at runtime
    }
    
private:
    struct EmbeddedFileInfo {
        size_t offset;
        size_t size;
    };
    
    const uint8_t* embedded_data_;
    std::unordered_map<std::string, EmbeddedFileInfo> embedded_files_;
    
    void ParseEmbeddedArchive(const uint8_t* data, size_t size);
};
```

#### Asset Loading Factory System

```cpp
// Type-safe asset loading with format detection
template<typename AssetType>
class AssetLoader {
public:
    virtual ~AssetLoader() = default;
    
    [[nodiscard]] virtual auto CanLoad(const std::string& path, 
                                      std::span<const uint8_t> data) const -> bool = 0;
    
    [[nodiscard]] virtual auto Load(const std::string& path, 
                                   std::span<const uint8_t> data) 
        -> std::expected<std::unique_ptr<AssetType>, AssetError> = 0;
    
    [[nodiscard]] virtual auto GetSupportedExtensions() const -> std::vector<std::string> = 0;
};

// JSON configuration loader
class JsonAssetLoader final : public AssetLoader<JsonAsset> {
public:
    [[nodiscard]] auto CanLoad(const std::string& path, 
                              std::span<const uint8_t> data) const -> bool override {
        return path.ends_with(".json") || IsValidJson(data);
    }
    
    [[nodiscard]] auto Load(const std::string& path, 
                           std::span<const uint8_t> data) 
        -> std::expected<std::unique_ptr<JsonAsset>, AssetError> override {
        
        try {
            std::string json_text(data.begin(), data.end());
            auto json_data = nlohmann::json::parse(json_text);
            
            auto asset = std::make_unique<JsonAsset>();
            asset->data = std::move(json_data);
            asset->source_path = path;
            
            return asset;
        } catch (const nlohmann::json::exception& e) {
            return std::unexpected(AssetError::ParseFailed);
        }
    }
    
    [[nodiscard]] auto GetSupportedExtensions() const -> std::vector<std::string> override {
        return {".json", ".config"};
    }
    
private:
    [[nodiscard]] auto IsValidJson(std::span<const uint8_t> data) const -> bool {
        try {
            std::string text(data.begin(), data.end());
            nlohmann::json::parse(text);
            return true;
        } catch (...) {
            return false;
        }
    }
};

// Texture asset loader with multiple format support
class TextureAssetLoader final : public AssetLoader<TextureAsset> {
public:
    [[nodiscard]] auto CanLoad(const std::string& path, 
                              std::span<const uint8_t> data) const -> bool override {
        const auto extensions = GetSupportedExtensions();
        return std::ranges::any_of(extensions, [&path](const std::string& ext) {
            return path.ends_with(ext);
        }) || HasImageSignature(data);
    }
    
    [[nodiscard]] auto Load(const std::string& path, 
                           std::span<const uint8_t> data) 
        -> std::expected<std::unique_ptr<TextureAsset>, AssetError> override {
        
        // Use stb_image or similar for texture loading
        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(
            data.data(), 
            static_cast<int>(data.size()), 
            &width, &height, &channels, 
            STBI_rgb_alpha
        );
        
        if (!pixels) {
            return std::unexpected(AssetError::ParseFailed);
        }
        
        auto asset = std::make_unique<TextureAsset>();
        asset->width = width;
        asset->height = height;
        asset->channels = 4; // Force RGBA
        asset->pixel_data.assign(pixels, pixels + (width * height * 4));
        asset->source_path = path;
        
        stbi_image_free(pixels);
        
        return asset;
    }
    
    [[nodiscard]] auto GetSupportedExtensions() const -> std::vector<std::string> override {
        return {".png", ".jpg", ".jpeg", ".bmp", ".tga"};
    }
    
private:
    [[nodiscard]] auto HasImageSignature(std::span<const uint8_t> data) const -> bool {
        if (data.size() < 8) return false;
        
        // Check for common image format signatures
        // PNG: 89 50 4E 47 0D 0A 1A 0A
        static constexpr std::array<uint8_t, 8> png_signature = 
            {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        
        return std::equal(png_signature.begin(), png_signature.end(), data.begin());
    }
};

// Asset loader factory for automatic format detection
class AssetLoaderFactory {
public:
    template<typename AssetType>
    void RegisterLoader(std::unique_ptr<AssetLoader<AssetType>> loader) {
        const auto type_id = std::type_index(typeid(AssetType));
        auto& loaders = loaders_[type_id];
        loaders.push_back(std::move(loader));
    }
    
    template<typename AssetType>
    [[nodiscard]] auto FindLoader(const std::string& path, 
                                 std::span<const uint8_t> data) const 
        -> const AssetLoader<AssetType>* {
        
        const auto type_id = std::type_index(typeid(AssetType));
        auto it = loaders_.find(type_id);
        if (it == loaders_.end()) {
            return nullptr;
        }
        
        for (const auto& loader : it->second) {
            auto typed_loader = static_cast<const AssetLoader<AssetType>*>(loader.get());
            if (typed_loader->CanLoad(path, data)) {
                return typed_loader;
            }
        }
        
        return nullptr;
    }
    
private:
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<void>>> loaders_;
};
```

### Hot-Reload and Dependency System

#### File System Monitoring

```cpp
// Cross-platform file system watching for hot-reload
class HotReloadWatcher {
public:
    explicit HotReloadWatcher(AssetManager& asset_manager) 
        : asset_manager_(asset_manager)
        , should_stop_(false) {
        
        #ifdef __EMSCRIPTEN__
            // WebAssembly: no file watching capability
            return;
        #endif
        
        watcher_thread_ = std::jthread([this](const std::stop_token& stop_token) {
            WatcherThreadFunction(stop_token);
        });
    }
    
    ~HotReloadWatcher() {
        should_stop_.store(true, std::memory_order_release);
        if (watcher_thread_.joinable()) {
            watcher_thread_.request_stop();
            watcher_thread_.join();
        }
    }
    
    void AddWatchPath(const std::filesystem::path& path) {
        std::unique_lock lock(watch_paths_mutex_);
        watch_paths_.insert(path);
        
        #if defined(_WIN32)
            AddWindowsWatch(path);
        #elif defined(__linux__)
            AddLinuxWatch(path);
        #endif
    }
    
    void RemoveWatchPath(const std::filesystem::path& path) {
        std::unique_lock lock(watch_paths_mutex_);
        watch_paths_.erase(path);
        
        #if defined(_WIN32)
            RemoveWindowsWatch(path);
        #elif defined(__linux__)
            RemoveLinuxWatch(path);
        #endif
    }
    
private:
    AssetManager& asset_manager_;
    std::atomic<bool> should_stop_;
    std::jthread watcher_thread_;
    
    std::mutex watch_paths_mutex_;
    std::unordered_set<std::filesystem::path> watch_paths_;
    
    // Platform-specific watching implementation
    #if defined(_WIN32)
        std::unordered_map<std::filesystem::path, HANDLE> windows_watches_;
        void AddWindowsWatch(const std::filesystem::path& path);
        void RemoveWindowsWatch(const std::filesystem::path& path);
    #elif defined(__linux__)
        int inotify_fd_ = -1;
        std::unordered_map<std::filesystem::path, int> linux_watches_;
        void AddLinuxWatch(const std::filesystem::path& path);
        void RemoveLinuxWatch(const std::filesystem::path& path);
    #endif
    
    void WatcherThreadFunction(const std::stop_token& stop_token);
    void ProcessFileChange(const std::filesystem::path& changed_path);
};
```

#### Dependency Tracking System

```cpp
// Asset dependency graph for incremental updates
class DependencyTracker {
public:
    void AddDependency(AssetId dependent, AssetId dependency) {
        std::unique_lock lock(graph_mutex_);
        
        dependencies_[dependent].insert(dependency);
        dependents_[dependency].insert(dependent);
    }
    
    void RemoveDependency(AssetId dependent, AssetId dependency) {
        std::unique_lock lock(graph_mutex_);
        
        dependencies_[dependent].erase(dependency);
        dependents_[dependency].erase(dependent);
    }
    
    void RemoveAsset(AssetId asset_id) {
        std::unique_lock lock(graph_mutex_);
        
        // Remove all dependencies of this asset
        if (auto it = dependencies_.find(asset_id); it != dependencies_.end()) {
            for (AssetId dependency : it->second) {
                dependents_[dependency].erase(asset_id);
            }
            dependencies_.erase(it);
        }
        
        // Remove this asset as a dependency of others
        if (auto it = dependents_.find(asset_id); it != dependents_.end()) {
            for (AssetId dependent : it->second) {
                dependencies_[dependent].erase(asset_id);
            }
            dependents_.erase(it);
        }
    }
    
    [[nodiscard]] auto GetDependents(AssetId asset_id) const -> std::vector<AssetId> {
        std::shared_lock lock(graph_mutex_);
        
        if (auto it = dependents_.find(asset_id); it != dependents_.end()) {
            return {it->second.begin(), it->second.end()};
        }
        
        return {};
    }
    
    [[nodiscard]] auto GetDependencies(AssetId asset_id) const -> std::vector<AssetId> {
        std::shared_lock lock(graph_mutex_);
        
        if (auto it = dependencies_.find(asset_id); it != dependencies_.end()) {
            return {it->second.begin(), it->second.end()};
        }
        
        return {};
    }
    
    // Topological sort for dependency-ordered loading
    [[nodiscard]] auto GetLoadOrder(const std::vector<AssetId>& assets) const 
        -> std::expected<std::vector<AssetId>, DependencyError> {
        
        std::shared_lock lock(graph_mutex_);
        
        std::vector<AssetId> result;
        std::unordered_set<AssetId> visited;
        std::unordered_set<AssetId> temp_visited;
        
        std::function<bool(AssetId)> visit = [&](AssetId asset_id) -> bool {
            if (temp_visited.contains(asset_id)) {
                return false; // Circular dependency detected
            }
            
            if (visited.contains(asset_id)) {
                return true; // Already processed
            }
            
            temp_visited.insert(asset_id);
            
            // Visit all dependencies first
            if (auto it = dependencies_.find(asset_id); it != dependencies_.end()) {
                for (AssetId dependency : it->second) {
                    if (!visit(dependency)) {
                        return false;
                    }
                }
            }
            
            temp_visited.erase(asset_id);
            visited.insert(asset_id);
            result.push_back(asset_id);
            
            return true;
        };
        
        // Process all requested assets
        for (AssetId asset_id : assets) {
            if (!visit(asset_id)) {
                return std::unexpected(DependencyError::CircularDependency);
            }
        }
        
        return result;
    }
    
private:
    mutable std::shared_mutex graph_mutex_;
    std::unordered_map<AssetId, std::unordered_set<AssetId>> dependencies_;
    std::unordered_map<AssetId, std::unordered_set<AssetId>> dependents_;
};
```

### ECS Integration

#### Asset Components for Entity System

```cpp
// Component for referencing assets from entities
template<typename AssetType>
struct AssetComponent {
    AssetHandle<AssetType> asset;
    bool auto_reload = true;
    
    // Optional override path for per-entity customization
    std::optional<std::string> override_path;
    
    [[nodiscard]] auto GetAsset() const -> const AssetType* {
        return asset.Get();
    }
    
    [[nodiscard]] auto IsLoaded() const -> bool {
        return asset.IsValid();
    }
};

// Component traits for ECS integration
template<typename AssetType>
struct ComponentTraits<AssetComponent<AssetType>> {
    static constexpr bool is_trivially_copyable = false; // Contains AssetHandle
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(AssetHandle<AssetType>);
};

// Specialized asset components for common types
using TextureComponent = AssetComponent<TextureAsset>;
using MeshComponent = AssetComponent<MeshAsset>;
using AudioComponent = AssetComponent<AudioAsset>;
using ConfigComponent = AssetComponent<JsonAsset>;
```

#### Asset Loading System for ECS

```cpp
// ECS system for managing entity asset loading
class AssetLoadingSystem final : public System {
public:
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {},
            .write_components = {
                GetComponentTypeId<TextureComponent>(),
                GetComponentTypeId<MeshComponent>(),
                GetComponentTypeId<AudioComponent>(),
                GetComponentTypeId<ConfigComponent>()
            },
            .execution_phase = ExecutionPhase::PreUpdate,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Process entities with pending asset loads
        ProcessPendingLoads<TextureComponent>(world);
        ProcessPendingLoads<MeshComponent>(world);
        ProcessPendingLoads<AudioComponent>(world);
        ProcessPendingLoads<ConfigComponent>(world);
        
        // Handle hot-reload updates
        if (AssetManager::Instance().HasPendingReloads()) {
            ProcessHotReloads(world);
        }
    }
    
private:
    template<typename AssetComponentType>
    void ProcessPendingLoads(World& world) {
        using AssetType = typename AssetComponentType::AssetType;
        
        world.ForEachComponent<AssetComponentType>([](EntityId entity, AssetComponentType& component) {
            if (!component.asset.IsValid() && component.override_path) {
                // Load asset using override path
                component.asset = AssetManager::Instance().LoadAsset<AssetType>(*component.override_path);
            }
        });
    }
    
    void ProcessHotReloads(World& world) {
        const auto reloaded_assets = AssetManager::Instance().GetReloadedAssets();
        
        for (AssetId asset_id : reloaded_assets) {
            // Find all entities using this asset and update them
            UpdateEntitiesUsingAsset(world, asset_id);
        }
    }
    
    void UpdateEntitiesUsingAsset(World& world, AssetId asset_id) {
        // Check texture components
        world.ForEachComponent<TextureComponent>([asset_id](EntityId entity, TextureComponent& component) {
            if (component.asset.GetId() == asset_id && component.auto_reload) {
                // Asset was reloaded - component will automatically reference new version
                OnAssetReloaded(entity, component);
            }
        });
        
        // Repeat for other asset component types...
    }
    
    template<typename AssetComponentType>
    void OnAssetReloaded(EntityId entity, AssetComponentType& component) {
        // Notify other systems that this entity's asset was reloaded
        // This could trigger GPU resource updates, etc.
    }
};
```

### Performance Optimization

#### Memory Management and Caching

```cpp
// LRU cache for memory-efficient asset management
template<typename AssetType>
class AssetCache {
public:
    explicit AssetCache(size_t max_memory_bytes) 
        : max_memory_bytes_(max_memory_bytes) {}
    
    void Insert(AssetId id, std::unique_ptr<AssetType> asset) {
        std::unique_lock lock(cache_mutex_);
        
        const size_t asset_size = CalculateAssetSize(*asset);
        
        // Evict assets if necessary to make room
        while (current_memory_usage_ + asset_size > max_memory_bytes_ && !lru_list_.empty()) {
            EvictLeastRecentlyUsed();
        }
        
        // Insert new asset
        auto node = std::make_shared<CacheNode>();
        node->id = id;
        node->asset = std::move(asset);
        node->size_bytes = asset_size;
        
        lru_list_.push_front(node);
        cache_map_[id] = lru_list_.begin();
        current_memory_usage_ += asset_size;
    }
    
    [[nodiscard]] auto Get(AssetId id) -> const AssetType* {
        std::unique_lock lock(cache_mutex_);
        
        auto it = cache_map_.find(id);
        if (it == cache_map_.end()) {
            return nullptr;
        }
        
        // Move to front of LRU list
        auto node = *it->second;
        lru_list_.erase(it->second);
        lru_list_.push_front(node);
        cache_map_[id] = lru_list_.begin();
        
        return node->asset.get();
    }
    
    void Remove(AssetId id) {
        std::unique_lock lock(cache_mutex_);
        
        auto it = cache_map_.find(id);
        if (it != cache_map_.end()) {
            auto node = *it->second;
            current_memory_usage_ -= node->size_bytes;
            lru_list_.erase(it->second);
            cache_map_.erase(it);
        }
    }
    
    [[nodiscard]] auto GetMemoryUsage() const -> size_t {
        std::shared_lock lock(cache_mutex_);
        return current_memory_usage_;
    }
    
    [[nodiscard]] auto GetCacheHitRatio() const -> float {
        std::shared_lock lock(cache_mutex_);
        if (total_requests_ == 0) return 0.0f;
        return static_cast<float>(cache_hits_) / static_cast<float>(total_requests_);
    }
    
private:
    struct CacheNode {
        AssetId id;
        std::unique_ptr<AssetType> asset;
        size_t size_bytes;
    };
    
    mutable std::shared_mutex cache_mutex_;
    std::list<std::shared_ptr<CacheNode>> lru_list_;
    std::unordered_map<AssetId, typename std::list<std::shared_ptr<CacheNode>>::iterator> cache_map_;
    
    size_t max_memory_bytes_;
    size_t current_memory_usage_ = 0;
    
    // Performance metrics
    mutable std::atomic<uint64_t> total_requests_{0};
    mutable std::atomic<uint64_t> cache_hits_{0};
    
    void EvictLeastRecentlyUsed() {
        if (!lru_list_.empty()) {
            auto node = lru_list_.back();
            current_memory_usage_ -= node->size_bytes;
            cache_map_.erase(node->id);
            lru_list_.pop_back();
        }
    }
    
    [[nodiscard]] auto CalculateAssetSize(const AssetType& asset) const -> size_t {
        if constexpr (std::is_same_v<AssetType, TextureAsset>) {
            return asset.pixel_data.size();
        } else if constexpr (std::is_same_v<AssetType, JsonAsset>) {
            return asset.data.dump().size();
        } else {
            return sizeof(AssetType);
        }
    }
};
```

#### Streaming and Lazy Loading

```cpp
// Asset streaming system for large worlds
class AssetStreamingManager {
public:
    explicit AssetStreamingManager(size_t stream_radius_meters = 1000) 
        : stream_radius_meters_(stream_radius_meters) {}
    
    void UpdateStreamingPosition(const glm::vec3& world_position) {
        current_position_ = world_position;
        
        // Schedule streaming updates on I/O thread pool
        AssetManager::Instance().ScheduleStreamingUpdate(world_position, stream_radius_meters_);
    }
    
    void RegisterStreamableAsset(AssetId asset_id, const glm::vec3& world_position, float radius = 100.0f) {
        std::unique_lock lock(streamable_assets_mutex_);
        
        streamable_assets_[asset_id] = StreamableAssetInfo{
            .world_position = world_position,
            .radius = radius,
            .is_loaded = false,
            .priority = CalculatePriority(world_position, radius)
        };
    }
    
    void UpdateStreamableAssets() {
        std::unique_lock lock(streamable_assets_mutex_);
        
        std::vector<AssetId> to_load;
        std::vector<AssetId> to_unload;
        
        for (auto& [asset_id, info] : streamable_assets_) {
            const float distance = glm::distance(current_position_, info.world_position);
            const bool should_be_loaded = distance <= (stream_radius_meters_ + info.radius);
            
            if (should_be_loaded && !info.is_loaded) {
                to_load.push_back(asset_id);
                info.priority = CalculatePriority(info.world_position, info.radius);
            } else if (!should_be_loaded && info.is_loaded) {
                to_unload.push_back(asset_id);
            }
        }
        
        // Sort by priority for optimal loading order
        std::ranges::sort(to_load, [this](AssetId a, AssetId b) {
            return streamable_assets_[a].priority > streamable_assets_[b].priority;
        });
        
        // Execute streaming operations
        for (AssetId asset_id : to_load) {
            ScheduleAssetLoad(asset_id);
        }
        
        for (AssetId asset_id : to_unload) {
            ScheduleAssetUnload(asset_id);
        }
    }
    
private:
    struct StreamableAssetInfo {
        glm::vec3 world_position;
        float radius;
        bool is_loaded;
        float priority;
    };
    
    std::mutex streamable_assets_mutex_;
    std::unordered_map<AssetId, StreamableAssetInfo> streamable_assets_;
    
    glm::vec3 current_position_{0.0f};
    float stream_radius_meters_;
    
    [[nodiscard]] auto CalculatePriority(const glm::vec3& asset_position, float asset_radius) const -> float {
        const float distance = glm::distance(current_position_, asset_position);
        const float normalized_distance = distance / (stream_radius_meters_ + asset_radius);
        return 1.0f - std::clamp(normalized_distance, 0.0f, 1.0f);
    }
    
    void ScheduleAssetLoad(AssetId asset_id) {
        // Integrate with I/O thread pool for background loading
    }
    
    void ScheduleAssetUnload(AssetId asset_id) {
        // Schedule for unload with delay to prevent thrashing
    }
};
```

## Success Criteria

### Performance Metrics

1. **Hot-Reload Speed**: Asset reloading completes within 1 second for typical game assets
2. **Memory Efficiency**: Peak memory usage remains under 512MB for standard gameplay scenarios
3. **Loading Performance**: 95% of assets load within frame budget (16ms) during gameplay
4. **Cross-Platform Parity**: Identical loading behavior between native and WebAssembly builds

### Functionality Requirements

1. **Asset Type Support**: JSON, textures, audio, meshes, and shaders all supported with extensible loader system
2. **Dependency Management**: Automatic dependency tracking with circular dependency detection
3. **Hot-Reload Integration**: File system monitoring with automatic asset invalidation and reloading
4. **ECS Integration**: Seamless component-based asset reference system with automatic updates

### Reliability Standards

1. **Error Handling**: Graceful degradation for missing, corrupted, or unsupported assets
2. **Memory Safety**: No memory leaks during asset loading, unloading, or hot-reload cycles
3. **Thread Safety**: All asset operations safe for concurrent access from multiple systems
4. **Platform Compatibility**: 100% feature parity between development and production builds

## Future Enhancements

### Phase 2: Advanced Asset Processing

#### Asset Build Pipeline Integration

```cpp
// Integration with external asset processing tools
class AssetBuildPipeline {
public:
    void RegisterProcessor(std::string_view asset_type, std::unique_ptr<AssetProcessor> processor);
    void ProcessAssets(const std::filesystem::path& source_dir, const std::filesystem::path& output_dir);
    void EnableIncrementalBuilds(bool enable);
    
private:
    // Coordinate with external tools for texture compression, mesh optimization, etc.
    void ProcessTextures();
    void ProcessAudio();
    void ProcessMeshes();
};
```

#### Asset Version Management

```cpp
// Automatic asset migration and version compatibility
class AssetVersionManager {
public:
    void RegisterMigration(uint32_t from_version, uint32_t to_version, AssetMigrationFunc migration);
    auto LoadAssetWithMigration(const std::string& path) -> std::expected<std::unique_ptr<Asset>, AssetError>;
    
private:
    // Handle asset format evolution over time
    std::unordered_map<std::pair<uint32_t, uint32_t>, AssetMigrationFunc> migrations_;
};
```

### Phase 3: Distributed Asset Systems

#### Network Asset Loading

```cpp
// Future extension for distributed asset systems
class NetworkAssetProvider final : public PlatformAssetProvider {
public:
    void SetAssetServer(const NetworkAddress& server_address);
    void EnableAssetCaching(bool enable);
    void SetCacheDirectory(const std::filesystem::path& cache_dir);
    
private:
    // Download assets from remote servers with local caching
    // Useful for game updates, DLC, or user-generated content
};
```

#### Content Management Integration

```cpp
// Integration with content management and deployment systems
class ContentDeploymentManager {
public:
    void CheckForUpdates();
    void DownloadAssetUpdates();
    void ApplyAssetUpdates();
    
private:
    // Coordinate with CDN and content deployment networks
    // Handle incremental updates and patch management
};
```

### Phase 4: Advanced Development Tools

#### Asset Dependency Visualization

```cpp
// Visual dependency graph for development tools
class AssetDependencyVisualizer {
public:
    void GenerateDependencyGraph(const std::string& output_format);
    void HighlightCircularDependencies();
    void ShowAssetUsageAnalytics();
    
private:
    // Generate visual representations of asset relationships
    // Integrate with ImGui for runtime dependency browser
};
```

#### Asset Performance Profiler

```cpp
// Comprehensive asset loading and usage profiling
class AssetProfiler {
public:
    void StartProfiling();
    void StopProfiling();
    void GenerateReport(const std::filesystem::path& output_path);
    
private:
    // Track asset loading times, memory usage, cache hit rates
    // Identify optimization opportunities and bottlenecks
};
```

## Risk Mitigation

### Hot-Reload Complexity Risks

- **Risk**: File system monitoring causing performance degradation or instability
- **Mitigation**: Optional hot-reload system that can be disabled for production builds
- **Fallback**: Manual asset reloading commands for development when automatic watching fails

### Cross-Platform Asset Differences

- **Risk**: Asset loading behaving differently between native and WebAssembly builds
- **Mitigation**: Comprehensive platform abstraction with identical high-level APIs
- **Fallback**: Platform-specific asset validation and conversion tools

### Memory Management Risks

- **Risk**: Asset caching consuming excessive memory or causing memory leaks
- **Mitigation**: LRU eviction with configurable memory limits and comprehensive leak testing
- **Fallback**: Disable caching entirely for memory-constrained environments

### Dependency Complexity Risks

- **Risk**: Complex asset dependency chains causing loading performance issues
- **Mitigation**: Dependency graph analysis with circular dependency detection
- **Fallback**: Manual dependency specification override for problematic asset chains

This asset pipeline system provides a robust foundation for game asset management while supporting the engine's
development workflow and cross-platform requirements. The design emphasizes extensibility and can evolve with the
engine's growing asset complexity needs.
