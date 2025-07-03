# MOD_PLATFORM_v1

> **Cross-Platform Abstraction Layer - Foundation Module**

## Executive Summary

The `engine.platform` module provides essential cross-platform abstractions for file I/O, memory management, timing, threading primitives, and system utilities. It serves as the zero-dependency foundation layer that enables consistent behavior across Windows, Linux, and WebAssembly platforms while maintaining zero-overhead abstractions and optimal performance characteristics for the Colony Game Engine's multi-platform deployment requirements.

## Scope and Objectives

### In Scope

- [ ] Cross-platform file system abstraction with async I/O
- [ ] High-performance memory management and custom allocators
- [ ] Precise timing and frame rate control systems
- [ ] Threading primitives and synchronization mechanisms
- [ ] System information and capability detection
- [ ] Platform-specific optimizations (SIMD, cache awareness)

### Out of Scope

- [ ] Graphics API abstraction (handled by engine.rendering)
- [ ] Audio API abstraction (handled by engine.audio)
- [ ] Network socket abstraction (handled by engine.networking)
- [ ] Game-specific file formats

### Primary Objectives

1. **Zero-Overhead Abstraction**: Platform abstractions with no performance penalty over native APIs
2. **WebAssembly Compatibility**: Full feature parity between native and WASM builds
3. **Memory Efficiency**: Custom allocators providing 20%+ performance improvement over std::allocator

### Secondary Objectives

- Hot-reload capable file watching across all platforms
- Sub-microsecond timing precision for profiling
- Lock-free primitives where platform-supported

## Architecture/Design

### ECS Architecture Design

#### Entity Management
- **Entity Creation/Destruction**: Platform module operates as a utility layer with no direct entity management
- **Entity Queries**: N/A - Platform provides foundational services to other systems
- **Component Dependencies**: No direct component dependencies; provides allocators and utilities for component storage

#### Component Design
```cpp
// Platform module provides utility components for other systems
struct PlatformInfo {
    Platform current_platform;
    std::uint32_t cpu_cores;
    std::uint32_t logical_processors;
    bool has_sse2;
    bool has_avx;
};

// Component traits for platform utilities
template<>
struct ComponentTraits<PlatformInfo> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 1; // Singleton resource
};
```

#### System Integration
- **System Dependencies**: All other engine systems depend on platform module
- **Component Access Patterns**: Read-only access to platform configuration data
- **Inter-System Communication**: Provides timing and threading primitives for system coordination

### Multi-Threading Design

#### Threading Model
- **Execution Phase**: Foundation - runs before all other systems
- **Thread Safety**: All platform APIs are thread-safe and lock-free where possible
- **Data Dependencies**: No dependencies on other systems; provides dependencies for others

#### Parallel Execution Strategy
```cpp
// Platform module provides threading primitives for other systems
class ThreadingPrimitives {
public:
    // Lock-free synchronization where supported
    [[nodiscard]] auto CreateSpinLock() const -> std::unique_ptr<ISpinLock>;
    [[nodiscard]] auto CreateAtomicCounter() const -> std::unique_ptr<IAtomicCounter>;
    
    // Platform-optimized thread creation
    [[nodiscard]] auto CreateThread(std::function<void()> func) const -> std::unique_ptr<IThread>;
};
```

#### Memory Access Patterns
- **Cache Efficiency**: All platform data structures designed for cache-line alignment
- **Memory Ordering**: Provides memory ordering guarantees for cross-platform consistency
- **Lock-Free Sections**: File I/O and timing operations are lock-free where platform allows

### Public APIs

#### Primary Interface: `PlatformInterface`
```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <chrono>

namespace engine::platform {

template<typename T>
concept Allocatable = requires(T t) {
    sizeof(T);
    alignof(T);
};

class PlatformInterface {
public:
    [[nodiscard]] auto Initialize() -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // File system operations
    [[nodiscard]] auto ReadFile(std::string_view path) -> std::optional<std::vector<std::uint8_t>>;
    [[nodiscard]] auto WriteFile(std::string_view path, std::span<const std::uint8_t> data) -> bool;
    
    // Memory management
    template<Allocatable T>
    [[nodiscard]] auto AllocateAligned(std::size_t count, std::size_t alignment = alignof(T)) -> T*;
    
    template<Allocatable T>
    void DeallocateAligned(T* ptr, std::size_t count) noexcept;
    
    // Timing operations
    [[nodiscard]] auto GetHighResolutionTime() const noexcept -> std::chrono::nanoseconds;
    void Sleep(std::chrono::nanoseconds duration) const noexcept;
    
    // System information
    [[nodiscard]] auto GetSystemInfo() const noexcept -> const SystemInfo&;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    bool is_initialized_ = false;
    SystemInfo cached_system_info_;
};

} // namespace engine::platform
```

#### Scripting Interface Requirements
```cpp
// Platform scripting interface for configuration access
class PlatformScriptInterface {
public:
    // Type-safe platform information access
    [[nodiscard]] auto GetPlatformName() const -> std::string_view;
    [[nodiscard]] auto GetCpuCoreCount() const -> std::uint32_t;
    [[nodiscard]] auto HasSimdSupport() const -> bool;
    
    // File system operations from scripts
    [[nodiscard]] auto FileExists(std::string_view path) const -> bool;
    [[nodiscard]] auto ReadTextFile(std::string_view path) -> std::optional<std::string>;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
};
```

## Module Dependencies

### **Standard Library Dependencies**

```cpp
import std.core;         // Standard library fundamentals, filesystem, chrono
// No engine module dependencies - this is the foundation layer
```

### **Dependency Rationale**

- **Standard Library Only**: As the foundation module, platform has no dependencies on other engine modules
- **Minimal Dependencies**: Uses only standard library components to ensure maximum portability
- **Platform Headers**: Platform-specific implementations include native APIs as needed

## Performance Characteristics

### **Zero-Overhead Abstractions**

- **Compile-Time Platform Detection**: No runtime overhead for platform-specific code paths
- **Inline Functions**: Hot path functions inlined for optimal performance
- **Template Specialization**: Platform-specific optimizations through template specialization
- **Static Polymorphism**: Compile-time dispatch instead of virtual function overhead

### **Memory Efficiency**

- **Stack Allocators**: High-performance linear allocation for temporary objects
- **Pool Allocators**: Reduced fragmentation for fixed-size object allocation
- **Memory Mapping**: Efficient large file access without loading into memory
- **WebAssembly Optimization**: Minimal heap usage for browser constraints

### **I/O Performance**

- **Buffered Operations**: Intelligent buffering for optimal throughput
- **Async File Operations**: Non-blocking file I/O where supported
- **Memory Mapping**: Direct memory access to file data
- **Batch Operations**: Directory operations batched for efficiency

## Usage Examples

### **File System Operations**

```cpp
import engine.platform;

// Read configuration file
auto& fs = engine::platform::FileSystem::instance();
auto config_result = fs.read_entire_file("config/settings.json");

if (config_result) {
    auto& config_data = *config_result;
    // Parse JSON data...
} else {
    // Handle error
    logger.error("Failed to read config file: {}", static_cast<int>(config_result.error()));
}

// Watch for file changes during development
if constexpr (engine::platform::get_current_platform() != engine::platform::Platform::WebAssembly) {
    auto watcher = fs.create_watcher("assets/", true);
    watcher->set_change_callback([](const std::string& path) {
        logger.info("File changed: {}", path);
        // Trigger hot-reload...
    });
    watcher->start();
}
```

### **Memory Management**

```cpp
// Use stack allocator for temporary objects
engine::platform::StackAllocator frame_allocator(1024 * 1024); // 1MB

void process_frame() {
    // Allocate temporary data on stack
    auto* temp_buffer = static_cast<float*>(
        frame_allocator.allocate(sizeof(float) * 1000));
    
    // Use temp_buffer for calculations...
    
    // Automatically cleaned up at end of frame
    frame_allocator.reset();
}

// Use pool allocator for frequent allocations
engine::platform::PoolAllocator<ParticleData> particle_pool(1000);

auto* particle = particle_pool.allocate(initial_position, initial_velocity);
// Use particle...
particle_pool.deallocate(particle);
```

### **High-Resolution Timing**

```cpp
// Frame timing
engine::platform::FrameTimer frame_timer(60.0); // Target 60 FPS

void game_loop() {
    while (running) {
        frame_timer.begin_frame();
        
        float delta_time = static_cast<float>(frame_timer.get_delta_time());
        
        // Update game logic
        update_game(delta_time);
        render_frame();
        
        frame_timer.end_frame();
        frame_timer.wait_for_next_frame(); // Frame limiting
    }
}

// Profiling with stopwatch
engine::platform::Stopwatch stopwatch;
stopwatch.start();

expensive_operation();

stopwatch.stop();
logger.info("Operation took {} ms", stopwatch.elapsed_milliseconds());
```

### **Platform-Specific Code**

```cpp
// Compile-time platform detection
if constexpr (engine::platform::is_web_platform()) {
    // WebAssembly-specific code
    engine::platform::web::set_canvas_size(800, 600);
    
    // Use IndexedDB for persistent storage
    auto storage = engine::platform::web::IndexedDB{};
    storage.store("save_game", save_data);
} else {
    // Desktop platform code
    auto save_path = fs.join_path(
        engine::platform::get_user_home_directory(),
        ".colony_game/saves/autosave.json"
    );
    fs.write_entire_file(save_path, save_data);
}

// Runtime system information
auto sys_info = engine::platform::get_system_info();
logger.info("Running on {} with {} cores", 
           sys_info.os_name, sys_info.cpu_cores);

if (sys_info.has_avx2) {
    // Use AVX2 optimized code paths
    enable_simd_optimizations();
}
```

## Integration Points

### **All Engine Modules**

- **Foundation Dependency**: Every other engine module imports platform for basic services
- **File I/O**: Asset loading, configuration, save data, shader source loading
- **Memory Management**: Custom allocators for performance-critical systems
- **Timing**: Frame rate control, animation timing, profiling measurements

### **Development Tools**

- **Hot-Reload**: File watching enables real-time asset and code reloading
- **Profiling**: High-resolution timing for performance analysis
- **Debug Logging**: Cross-platform logging for development and debugging
- **Crash Reporting**: Platform-specific crash handling and stack traces

### **WebAssembly Integration**

- **Browser APIs**: Canvas manipulation, fullscreen control, local storage
- **Web Workers**: Background processing for threading simulation
- **Memory Constraints**: Efficient memory usage for browser heap limitations
- **File Operations**: Fetch API integration for asset loading

## Testing Strategy

### **Unit Tests**

```cpp
// File system tests
TEST(PlatformTest, FileSystemOperations) {
    auto& fs = engine::platform::FileSystem::instance();
    
    // Test file creation and reading
    std::vector<std::uint8_t> test_data{'H', 'e', 'l', 'l', 'o'};
    auto write_result = fs.write_entire_file("test_file.txt", test_data);
    EXPECT_TRUE(write_result == engine::platform::FileResult::Success);
    
    auto read_result = fs.read_entire_file("test_file.txt");
    ASSERT_TRUE(read_result.has_value());
    EXPECT_EQ(*read_result, test_data);
    
    // Cleanup
    std::filesystem::remove("test_file.txt");
}

// Memory allocator tests
TEST(PlatformTest, StackAllocator) {
    engine::platform::StackAllocator allocator(1024);
    
    auto* ptr1 = allocator.allocate(100);
    auto* ptr2 = allocator.allocate(200);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_EQ(allocator.get_used(), 300);
    
    allocator.reset();
    EXPECT_EQ(allocator.get_used(), 0);
}

// Timer accuracy tests
TEST(PlatformTest, TimerAccuracy) {
    auto start = engine::platform::Timer::now();
    engine::platform::Timer::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = engine::platform::Timer::milliseconds_since(start);
    
    // Allow 20% tolerance for timing accuracy
    EXPECT_NEAR(elapsed, 10.0, 2.0);
}
```

### **Cross-Platform Tests**

```cpp
// Platform detection tests
TEST(PlatformTest, PlatformDetection) {
    auto platform = engine::platform::get_current_platform();
    EXPECT_NE(platform, engine::platform::Platform::Unknown);
    
    #ifdef _WIN32
    EXPECT_EQ(platform, engine::platform::Platform::Windows);
    #elif defined(__linux__)
    EXPECT_EQ(platform, engine::platform::Platform::Linux);
    #elif defined(__EMSCRIPTEN__)
    EXPECT_EQ(platform, engine::platform::Platform::WebAssembly);
    #endif
}

// System information tests
TEST(PlatformTest, SystemInformation) {
    auto sys_info = engine::platform::get_system_info();
    
    EXPECT_FALSE(sys_info.os_name.empty());
    EXPECT_GT(sys_info.cpu_cores, 0);
    EXPECT_GT(sys_info.logical_processors, 0);
    
    auto mem_info = engine::platform::MemoryManager::instance().get_memory_info();
    EXPECT_GT(mem_info.total_physical, 0);
}
```

## Cross-Platform Considerations

### **Platform-Specific Implementations**

#### **Windows**

- **File System**: Win32 APIs for optimal performance and Unicode support
- **Memory**: VirtualAlloc for large allocations, HeapAlloc for small objects
- **Timing**: QueryPerformanceCounter for microsecond precision
- **Threading**: Native Windows threads with proper synchronization

#### **Linux**

- **File System**: POSIX APIs with proper error handling and permissions
- **Memory**: mmap for memory mapping, malloc for general allocation
- **Timing**: clock_gettime with CLOCK_MONOTONIC for accuracy
- **Threading**: pthreads with proper cleanup and signal handling

#### **WebAssembly**

- **File System**: Fetch API for loading, IndexedDB for persistence
- **Memory**: WebAssembly.Memory management with heap monitoring
- **Timing**: performance.now() for high-resolution timing
- **Threading**: Web Workers with message passing simulation

### **Conditional Compilation**

```cpp
// Platform-specific optimizations
#if defined(_WIN32)
    // Windows-specific implementations
#elif defined(__linux__)
    // Linux-specific implementations  
#elif defined(__EMSCRIPTEN__)
    // WebAssembly-specific implementations
#else
    #error "Unsupported platform"
#endif
```

## Future Extensions

### **Additional Platforms**

- **macOS**: Native macOS support with Cocoa integration
- **iOS/Android**: Mobile platform support for future expansion
- **Game Consoles**: Console-specific optimizations and APIs
- **Cloud Gaming**: Streaming-optimized implementations

### **Advanced Features**

- **GPU Memory Management**: CUDA/OpenCL memory allocation abstraction
- **Network File Systems**: Remote file access and caching
- **Compression**: Built-in compression for file operations
- **Encryption**: File encryption for secure asset storage
- **Virtualization**: Docker and container-aware implementations

This platform module provides the essential foundation that enables all other engine systems to operate consistently
across different platforms while maintaining optimal performance characteristics.
