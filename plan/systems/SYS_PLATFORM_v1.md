# SYS_PLATFORM_v1

> **System-Level Design Document for Cross-Platform Abstraction Layer**

## Executive Summary

This document defines a comprehensive platform abstraction system for the modern C++20 game engine, designed to provide
unified OS services and cross-platform utilities that complement the existing GLFW3 windowing layer. The system
abstracts file I/O, high-resolution timing, memory management, threading primitives, and WebAssembly-specific
adaptations while maintaining identical API behavior across Windows, Linux, and WebAssembly platforms. The design
emphasizes zero-overhead abstractions, compile-time platform detection, and seamless integration with all engine systems
that require OS-level services.

## Scope and Objectives

### In Scope

- [ ] File system abstraction for native filesystem and WebAssembly embedded/fetch operations
- [ ] High-resolution timing and profiling utilities beyond GLFW's basic timer functions
- [ ] Memory management utilities including system memory queries and WebAssembly heap monitoring
- [ ] Threading primitives abstraction for native threads and WebAssembly Web Workers
- [ ] Debug logging and crash reporting with platform-specific implementations
- [ ] System resource monitoring (CPU cores, available memory, platform identification)
- [ ] Directory watching for development-time hot-reload capabilities
- [ ] Dynamic library loading for plugin systems and runtime extensions
- [ ] WebAssembly-specific browser API integration and fallback handling
- [ ] Development tools integration for debugging and profiling workflows

### Out of Scope

- [ ] Window management and OpenGL context creation (handled by existing GLFW3 integration)
- [ ] Input event handling (handled by GLFW3 callbacks and Input System)
- [ ] Network socket abstraction (future networking extension)
- [ ] Graphics API abstraction (handled by Rendering System)
- [ ] Audio device management (handled by Audio System backends)
- [ ] Platform-specific UI frameworks integration (external tooling approach)

### Primary Objectives

1. **Cross-Platform Parity**: Identical API behavior and feature availability across all target platforms
2. **Zero Overhead**: Compile-time platform detection with no runtime performance penalty
3. **WebAssembly Compatibility**: Seamless operation within browser security and memory constraints
4. **Development Workflow**: Hot-reload and debugging capabilities for optimal developer experience
5. **Foundation Stability**: Reliable OS services foundation for all dependent engine systems

### Secondary Objectives

- Future extensibility for additional platforms and OS services
- Comprehensive error handling with graceful degradation for unsupported features
- Memory-efficient operation suitable for WebAssembly heap constraints
- Integration with existing GLFW3 layer without breaking changes
- Performance profiling hooks for engine optimization and debugging

## Architecture/Design

### High-Level Overview

```
Platform Abstraction Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                   Engine Systems Layer                          │
│  Threading │ Assets │ Input │ Audio │ Rendering │ ECS │ Scene   │
│                                                                 │
│  All systems use Platform Abstraction for OS services          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Platform Abstraction Unified API                   │
├─────────────────────────────────────────────────────────────────┤
│  FileSystem │ Timer │ Memory │ Threading │ Debug │ System      │
│  Operations │ Utils │ Mgmt   │ Utils     │ Tools │ Info        │
│             │       │        │           │       │             │
│  Read/Write │ HiRes │ Alloc  │ Threads   │ Log   │ CPU/Memory  │
│  Directory  │ Clock │ Query  │ Sync      │ Crash │ Detection   │
│  Watching   │ Profil│ WebASM │ WebWorker │ Break │ Platform    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│               Platform Implementation Layer                      │
├─────────────────────────────────────────────────────────────────┤
│   Native Platforms          │         WebAssembly Platform     │
│                              │                                   │
│  Windows │ Linux │ macOS    │  Emscripten │ Browser APIs        │
│          │       │          │             │                     │
│  Win32   │ POSIX │ Cocoa    │  WASM       │ Web Workers         │
│  APIs    │ APIs  │ APIs     │  Runtime    │ Fetch API           │
│          │       │          │             │ IndexedDB           │
│  File    │ inotify│ FSEvents │  Virtual    │ Performance API     │
│  Watcher │       │          │  File Sys   │ Memory API          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  GLFW3 Integration Layer                        │
│                                                                 │
│  Existing engine::os::OS module continues to handle:           │
│  • Window creation and management                              │
│  • OpenGL context creation and management                      │
│  • Input event callbacks (keyboard, mouse, controller)         │
│  • Basic GLFW timing functions                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: File System Abstraction

- **Purpose**: Unified file operations across native filesystem and WebAssembly embedded/fetch mechanisms
- **Responsibilities**: File reading/writing, directory operations, path manipulation, hot-reload file watching
- **Key Classes/Interfaces**: `platform::FileSystem`, `FileWatcher`, `PathUtils`, `EmbeddedAssets`
- **Data Flow**: Engine requests → Platform detection → Native file ops OR WebAssembly fetch/embedded → Result

#### Component 2: High-Resolution Timer

- **Purpose**: Precise timing and profiling capabilities beyond GLFW's basic timer functions
- **Responsibilities**: Nanosecond-precision timing, frame profiling, performance measurements, sleep operations
- **Key Classes/Interfaces**: `platform::Timer`, `PerformanceCounter`, `ProfileScope`, `SleepUtils`
- **Data Flow**: Timing requests → Platform-specific high-res counters → Standardized time values

#### Component 3: Memory Management Utilities

- **Purpose**: System memory queries and WebAssembly heap monitoring for optimal memory usage
- **Responsibilities**: Available memory detection, allocation tracking, WebAssembly heap limits, memory mapping
- **Key Classes/Interfaces**: `platform::Memory`, `SystemMemory`, `WebAssemblyHeap`, `MemoryMapper`
- **Data Flow**: Memory queries → Platform memory APIs → Standardized memory information

#### Component 4: Threading Abstraction

- **Purpose**: Unified threading interface supporting native threads and WebAssembly Web Workers
- **Responsibilities**: Thread creation, synchronization primitives, CPU core detection, WebAssembly worker management
- **Key Classes/Interfaces**: `platform::Threading`, `ThreadPool`, `WebWorkerManager`, `SyncPrimitives`
- **Data Flow**: Threading requests → Platform detection → Native threads OR Web Workers → Execution

### Key Design Principles

#### Compile-Time Platform Detection

The platform abstraction uses compile-time detection to eliminate runtime overhead:

```cpp
namespace engine::platform {
    // Compile-time platform detection
    #if defined(__EMSCRIPTEN__)
        constexpr PlatformType CURRENT_PLATFORM = PlatformType::WebAssembly;
    #elif defined(_WIN32)
        constexpr PlatformType CURRENT_PLATFORM = PlatformType::Windows;
    #elif defined(__linux__)
        constexpr PlatformType CURRENT_PLATFORM = PlatformType::Linux;
    #elif defined(__APPLE__)
        constexpr PlatformType CURRENT_PLATFORM = PlatformType::macOS;
    #endif

    // Zero-overhead platform-specific implementations
    template<PlatformType Platform>
    class FileSystemImpl;
    
    template<PlatformType Platform>
    class TimerImpl;
    
    // Public API uses current platform automatically
    using FileSystem = FileSystemImpl<CURRENT_PLATFORM>;
    using Timer = TimerImpl<CURRENT_PLATFORM>;
}
```

#### FileSystem Abstraction Pattern

The file system abstraction handles the major differences between platforms:

```cpp
class FileSystem {
public:
    // Unified file operations
    [[nodiscard]] static std::optional<std::vector<uint8_t>> ReadFile(const std::string& path);
    [[nodiscard]] static bool WriteFile(const std::string& path, const std::vector<uint8_t>& data);
    [[nodiscard]] static std::vector<std::string> ListDirectory(const std::string& path);
    [[nodiscard]] static bool Exists(const std::string& path);
    [[nodiscard]] static bool CreateDirectory(const std::string& path);
    
    // Development-time file watching for hot-reload
    [[nodiscard]] static std::unique_ptr<FileWatcher> CreateWatcher(
        const std::string& directory,
        std::function<void(const std::string&)> callback
    );
    
    // WebAssembly-specific embedded asset access
    [[nodiscard]] static std::optional<std::vector<uint8_t>> ReadEmbeddedAsset(const std::string& path);
    [[nodiscard]] static bool IsEmbeddedAssetAvailable(const std::string& path);
};

// Platform-specific implementations
template<>
class FileSystemImpl<PlatformType::Windows> {
    // Windows-specific file operations using Win32 APIs
    // File watching using ReadDirectoryChangesW
};

template<>
class FileSystemImpl<PlatformType::Linux> {
    // Linux-specific file operations using POSIX APIs
    // File watching using inotify
};

template<>
class FileSystemImpl<PlatformType::WebAssembly> {
    // WebAssembly file operations using Emscripten APIs
    // Embedded asset access via --preload-file
    // No file watching (development-only feature)
};
```

#### WebAssembly Integration Strategy

The platform abstraction provides seamless WebAssembly integration:

```cpp
namespace engine::platform::webassembly {
    // WebAssembly-specific utilities
    class WebAssemblyUtils {
    public:
        [[nodiscard]] static bool IsWebWorkersAvailable();
        [[nodiscard]] static size_t GetHeapSize();
        [[nodiscard]] static size_t GetMaxHeapSize();
        [[nodiscard]] static bool IsSharedArrayBufferAvailable();
        
        // Browser API integration
        [[nodiscard]] static bool RequestPointerLock();
        static void ReleasePointerLock();
        [[nodiscard]] static bool IsFullscreenAvailable();
    };
    
    // WebAssembly-specific threading
    class WebWorkerManager {
    public:
        [[nodiscard]] static std::optional<WebWorkerHandle> CreateWorker(const std::string& script_url);
        static void PostMessage(WebWorkerHandle worker, const std::vector<uint8_t>& data);
        static void TerminateWorker(WebWorkerHandle worker);
    };
}
```

## Integration with Existing GLFW Layer

The Platform Abstraction system is designed to complement your existing `engine::os::OS` module:

### Preserved GLFW Responsibilities

Your existing `engine::os::OS` class continues to handle:

- Window creation via `CreateWindow()`
- OpenGL context management via `MakeContextCurrent()`
- Input callbacks via `SetKeyCallback()`, `SetMouseButtonCallback()`, etc.
- Basic window lifecycle via `Initialize()` and `Shutdown()`

### New Platform Abstraction Responsibilities

The Platform Abstraction adds the missing OS services:

- File I/O operations for asset loading and configuration
- High-resolution timing for performance profiling
- Memory management and system resource queries
- Threading utilities beyond basic GLFW support
- Development tools like file watching for hot-reload

### Integration Example

```cpp
// engine::os::OS handles GLFW operations
engine::os::OS::Initialize();
auto* window = engine::os::OS::CreateWindow(1920, 1080, "Colony Game");
engine::os::OS::MakeContextCurrent(window);

// platform:: handles everything else
auto config_data = engine::platform::FileSystem::ReadFile("config.json");
auto start_time = engine::platform::Timer::GetHighResolutionTicks();
auto available_memory = engine::platform::Memory::GetAvailableBytes();
auto cpu_cores = engine::platform::System::GetCPUCoreCount();
```

## Performance Requirements

### Target Specifications

- **File Operations**: <1ms for typical config file reads, <16ms for asset files
- **Timer Precision**: Nanosecond precision for profiling, microsecond accuracy for frame timing
- **Memory Overhead**: <1MB memory usage for platform abstraction utilities
- **WebAssembly Compatibility**: Zero performance degradation compared to native implementations
- **Hot-Reload Latency**: <100ms detection time for file changes during development

### Cross-Platform Feature Parity

| Feature            | Windows | Linux | macOS | WebAssembly      |
|--------------------|---------|-------|-------|------------------|
| File I/O           | Full    | Full  | Full  | Embedded/Fetch   |
| Directory Watching | Full    | Full  | Full  | Development Only |
| High-Res Timing    | Full    | Full  | Full  | Performance API  |
| Threading          | Full    | Full  | Full  | Web Workers      |
| Memory Queries     | Full    | Full  | Full  | Heap Monitoring  |
| Dynamic Loading    | Full    | Full  | Full  | Not Available    |

## Integration Points

### Threading System Dependencies

The Threading System requires platform abstraction for:

- Thread creation and management across platforms
- CPU core detection for optimal thread pool sizing
- WebAssembly Web Worker integration and fallback handling
- High-resolution timing for job scheduling and profiling

### Asset System Dependencies

The Asset System requires platform abstraction for:

- Cross-platform file I/O for asset loading
- Directory watching for hot-reload capabilities
- Memory management for asset streaming and caching
- WebAssembly embedded asset access

### Input System Dependencies

The Input System requires platform abstraction for:

- Configuration file loading for input mapping
- Hot-reload watching for input configuration changes
- High-resolution timing for input latency measurement
- WebAssembly browser API integration for advanced input features

### All Other Systems

Every engine system benefits from platform abstraction for:

- Debug logging and crash reporting
- Performance profiling and optimization
- Configuration file management
- Development workflow tools

This platform abstraction system completes the engine foundation by providing the fundamental OS services that all other
systems require, while maintaining the existing GLFW3 integration for windowing and basic input handling.
