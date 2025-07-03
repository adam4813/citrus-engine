# SYS_PROFILING_v1

> **System-Level Design Document for Development and Optimization Tools**

## Executive Summary

This document defines a comprehensive profiling and development tools system for the modern C++20 game engine, designed
to provide real-time performance monitoring, debugging capabilities, and optimization insights across all engine
systems. The system emphasizes minimal runtime overhead when enabled, zero overhead when disabled, and comprehensive
integration with all foundation and engine systems. The design supports both development-time profiling for optimization
workflows and optional lightweight telemetry for production builds, while maintaining cross-platform compatibility and
seamless integration with the scripting system for custom profiling logic.

## Scope and Objectives

### In Scope

- [ ] Real-time CPU profiling with hierarchical timing and bottleneck identification
- [ ] Memory allocation tracking and leak detection with call stack attribution
- [ ] Frame timing analysis with frame rate monitoring and spike detection
- [ ] ECS system performance monitoring with component query optimization insights
- [ ] Threading system analysis including job queue utilization and worker thread efficiency
- [ ] GPU performance monitoring with draw call tracking and shader profiling
- [ ] Asset loading performance analysis with hot-reload timing measurements
- [ ] Network performance monitoring including bandwidth and latency tracking
- [ ] Cross-platform profiling support with platform-specific optimization hooks
- [ ] Scripting interface for custom profiling metrics and performance triggers

### Out of Scope

- [ ] External profiler integration requiring complex third-party SDK dependencies
- [ ] Real-time code coverage analysis or advanced static analysis features
- [ ] Production telemetry collection or cloud-based analytics infrastructure
- [ ] Advanced GPU debugging features requiring graphics driver debugging APIs
- [ ] Binary instrumentation or runtime code modification capabilities
- [ ] Platform-specific profiling tools integration (Intel VTune, NVIDIA Nsight, etc.)

### Primary Objectives

1. **Zero Overhead**: Profiling system adds no performance cost when disabled in release builds
2. **Real-Time Insights**: Provide immediate feedback on performance bottlenecks during development
3. **System Integration**: Hook into all engine systems for comprehensive performance visibility
4. **Cross-Platform**: Identical profiling capabilities across Windows, Linux, and WebAssembly
5. **Developer Workflow**: Seamless integration with development tools and debugging workflows

### Secondary Objectives

- Hot-reload support for profiling configuration and custom metrics definitions
- Comprehensive data export capabilities for external analysis and visualization
- Future extensibility for advanced profiling features and optimization recommendations
- Memory-efficient operation suitable for WebAssembly development builds
- Integration with existing ImGui development UI for real-time visualization

## Architecture/Design

### High-Level Overview

```
Profiling System Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                 Development Interface Layer                     │
├─────────────────────────────────────────────────────────────────┤
│  ImGui         │ Script API    │ Export        │ Hot-Reload     │
│  Dashboard     │               │ System        │ Config         │
│                │               │               │                │
│  Real-Time     │ Lua/Python/   │ JSON/CSV      │ Profiling      │
│  Graphs &      │ AngelScript   │ Binary        │ Settings       │
│  Statistics    │ Integration   │ Formats       │ Updates        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│               Profiling Data Collection Layer                   │
├─────────────────────────────────────────────────────────────────┤
│  CPU Profiler  │ Memory        │ Frame         │ Custom         │
│                │ Tracker       │ Analyzer      │ Metrics        │
│                │               │               │                │
│  Hierarchical  │ Allocation    │ Frame Rate    │ User-Defined   │
│  Timing        │ Tracking      │ Spike         │ Counters &     │
│  Call Stacks   │ Leak Detection│ Detection     │ Timers         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              System Integration Hooks                           │
├─────────────────────────────────────────────────────────────────┤
│  ECS System    │ Threading     │ Rendering     │ Asset          │
│  Monitoring    │ Analysis      │ Stats         │ Loading        │
│                │               │               │                │
│  Component     │ Job Queue     │ Draw Calls    │ Load Times     │
│  Query Times   │ Utilization   │ GPU Stats     │ Hot-Reload     │
│  Entity Counts │ Worker Idle   │ Shader Perf   │ Performance    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│             Cross-Platform Implementation                       │
├─────────────────────────────────────────────────────────────────┤
│  Native        │ WebAssembly   │ Compile-Time  │ Data           │
│  Profiling     │ Profiling     │ Optimization  │ Storage        │
│                │               │               │                │
│  High-Res      │ Performance   │ PROFILE_SCOPE │ Ring Buffers   │
│  Timers        │ API           │ Macros        │ Lock-Free      │
│  Call Stacks   │ Memory API    │ Zero Overhead │ Collections    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                Foundation Integration                           │
├─────────────────────────────────────────────────────────────────┤
│  Platform      │ Threading     │ ECS Core      │ All Engine     │
│  Abstraction   │ System        │               │ Systems        │
│                │               │               │                │
│  High-Res      │ Job System    │ Component     │ Physics        │
│  Timing        │ Integration   │ Queries       │ Audio          │
│  Memory APIs   │ Worker Stats  │ System Stats  │ Networking     │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Performance Data Collection Engine

- **Purpose**: Real-time collection of performance metrics from all engine systems with minimal overhead
- **Responsibilities**: CPU timing, memory tracking, frame analysis, custom metric collection, data aggregation
- **Key Classes/Interfaces**: `Profiler`, `ProfileScope`, `MemoryTracker`, `FrameAnalyzer`, `MetricCollector`
- **Data Flow**: System operations → Profile hooks → Data collection → Aggregation → Storage → Visualization

#### Component 2: System Integration Hooks

- **Purpose**: Seamless integration with all engine systems to provide comprehensive performance visibility
- **Responsibilities**: ECS monitoring, threading analysis, rendering stats, asset performance, network metrics
- **Key Classes/Interfaces**: `SystemProfiler`, `ECSProfiler`, `ThreadingProfiler`, `RenderingProfiler`
- **Data Flow**: Engine systems → Profile points → Performance data → System-specific analysis → Unified reporting

#### Component 3: Development Interface and Visualization

- **Purpose**: Real-time performance visualization and debugging tools for development workflows
- **Responsibilities**: ImGui dashboard, graph rendering, data export, configuration management, alert systems
- **Key Classes/Interfaces**: `ProfilerUI`, `PerformanceGraph`, `ProfilerExporter`, `ProfilerConfig`
- **Data Flow**: Collected data → Visualization processing → ImGui rendering → Developer interaction → Configuration
  updates

#### Component 4: Cross-Platform Profiling Implementation

- **Purpose**: Platform-agnostic profiling infrastructure with platform-specific optimizations
- **Responsibilities**: High-resolution timing, memory APIs, call stack collection, WebAssembly adaptations
- **Key Classes/Interfaces**: `PlatformProfiler`, `ProfilerTimer`, `CallStackCollector`, `MemoryProfiler`
- **Data Flow**: Platform APIs → Abstraction layer → Unified profiling interface → Cross-platform compatibility

### CPU Profiling Architecture

#### Hierarchical Timing System

```cpp
namespace engine::profiling {

// Zero-overhead profiling macros with compile-time disabling
#ifdef PROFILING_ENABLED
    #define PROFILE_SCOPE(name) ProfileScope profile_scope_##__LINE__(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
    #define PROFILE_BEGIN(name) Profiler::Instance().BeginSection(name)
    #define PROFILE_END(name) Profiler::Instance().EndSection(name)
    #define PROFILE_COUNTER(name, value) Profiler::Instance().RecordCounter(name, value)
#else
    #define PROFILE_SCOPE(name) ((void)0)
    #define PROFILE_FUNCTION() ((void)0)
    #define PROFILE_BEGIN(name) ((void)0)
    #define PROFILE_END(name) ((void)0)
    #define PROFILE_COUNTER(name, value) ((void)0)
#endif

// RAII-based profile scope for automatic timing
class ProfileScope {
public:
    explicit ProfileScope(std::string_view name) 
        : name_(name)
        , start_time_(std::chrono::high_resolution_clock::now()) {
        Profiler::Instance().BeginSection(name_);
    }
    
    ~ProfileScope() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        Profiler::Instance().EndSection(name_, duration);
    }
    
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    ProfileScope(ProfileScope&&) = delete;
    ProfileScope& operator=(ProfileScope&&) = delete;

private:
    std::string_view name_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

// Thread-safe profiler with lock-free data collection
class Profiler {
public:
    static auto Instance() -> Profiler& {
        static Profiler instance;
        return instance;
    }
    
    // Enable/disable profiling at runtime
    void SetEnabled(bool enabled) { is_enabled_.store(enabled, std::memory_order_release); }
    [[nodiscard]] auto IsEnabled() const -> bool { return is_enabled_.load(std::memory_order_acquire); }
    
    // Profile section management
    void BeginSection(std::string_view name);
    void EndSection(std::string_view name, std::chrono::microseconds duration);
    void EndSection(std::string_view name); // Calculate duration automatically
    
    // Custom metrics
    void RecordCounter(std::string_view name, int64_t value);
    void RecordGauge(std::string_view name, double value);
    void RecordHistogram(std::string_view name, double value);
    
    // Frame boundaries for frame-based analysis
    void BeginFrame();
    void EndFrame();
    
    // Data access for visualization
    [[nodiscard]] auto GetCurrentFrameData() const -> const FrameProfileData&;
    [[nodiscard]] auto GetHistoricalData(size_t frame_count) const -> std::vector<FrameProfileData>;
    [[nodiscard]] auto GetSystemStats() const -> SystemProfileStats;
    
    // Configuration and export
    void LoadConfiguration(const ProfilerConfig& config);
    void ExportData(const std::filesystem::path& output_path, ExportFormat format) const;
    void ClearData();

private:
    std::atomic<bool> is_enabled_{false};
    
    // Thread-local storage for lock-free profiling
    thread_local static ProfileThreadData thread_data_;
    
    // Frame data storage
    std::array<FrameProfileData, 300> frame_history_; // 5 seconds at 60 FPS
    std::atomic<size_t> current_frame_index_{0};
    
    // System-wide statistics
    SystemProfileStats system_stats_;
    mutable std::shared_mutex stats_mutex_;
    
    Profiler() = default;
    ~Profiler() = default;
    
    void ProcessThreadData();
    void UpdateSystemStats();
};

// Thread-local profiling data for lock-free collection
struct ProfileThreadData {
    struct SectionData {
        std::string name;
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::microseconds total_duration{0};
        uint32_t call_count = 0;
        uint32_t max_depth = 0;
    };
    
    std::vector<SectionData> active_sections;
    std::unordered_map<std::string, SectionData> completed_sections;
    std::chrono::high_resolution_clock::time_point frame_start_time;
    uint32_t current_depth = 0;
    
    void Reset() {
        active_sections.clear();
        completed_sections.clear();
        current_depth = 0;
    }
};

// Frame-based profiling data
struct FrameProfileData {
    uint64_t frame_number = 0;
    std::chrono::microseconds frame_duration{0};
    std::chrono::high_resolution_clock::time_point frame_start_time;
    
    // System timings
    std::unordered_map<std::string, ProfileSectionData> sections;
    
    // Engine system stats
    ECSProfileData ecs_stats;
    ThreadingProfileData threading_stats;
    RenderingProfileData rendering_stats;
    AssetProfileData asset_stats;
    NetworkProfileData network_stats;
    
    // Custom metrics
    std::unordered_map<std::string, int64_t> counters;
    std::unordered_map<std::string, double> gauges;
    std::unordered_map<std::string, std::vector<double>> histograms;
};

// Individual profile section data
struct ProfileSectionData {
    std::chrono::microseconds total_time{0};
    std::chrono::microseconds min_time{std::chrono::microseconds::max()};
    std::chrono::microseconds max_time{0};
    uint32_t call_count = 0;
    uint32_t max_depth = 0;
    
    [[nodiscard]] auto GetAverageTime() const -> std::chrono::microseconds {
        return call_count > 0 ? total_time / call_count : std::chrono::microseconds{0};
    }
};

} // namespace engine::profiling
```

### Memory Profiling System

#### Allocation Tracking and Leak Detection

```cpp
namespace engine::profiling {

// Memory allocation tracking with call stack attribution
class MemoryProfiler {
public:
    static auto Instance() -> MemoryProfiler& {
        static MemoryProfiler instance;
        return instance;
    }
    
    // Memory tracking interface
    void RecordAllocation(void* ptr, size_t size, const char* file, int line);
    void RecordDeallocation(void* ptr);
    void RecordReallocation(void* old_ptr, void* new_ptr, size_t new_size, const char* file, int line);
    
    // Memory statistics
    [[nodiscard]] auto GetCurrentMemoryUsage() const -> MemoryStats;
    [[nodiscard]] auto GetAllocationHistory() const -> std::vector<AllocationEvent>;
    [[nodiscard]] auto DetectLeaks() const -> std::vector<MemoryLeak>;
    
    // Memory pool tracking
    void RegisterMemoryPool(const std::string& name, void* pool_start, size_t pool_size);
    void RecordPoolAllocation(const std::string& pool_name, void* ptr, size_t size);
    void RecordPoolDeallocation(const std::string& pool_name, void* ptr);
    
    // WebAssembly heap monitoring
    [[nodiscard]] auto GetWebAssemblyHeapUsage() const -> WebAssemblyMemoryStats;
    void SetWebAssemblyHeapLimit(size_t limit_bytes);

private:
    mutable std::shared_mutex allocations_mutex_;
    std::unordered_map<void*, AllocationInfo> active_allocations_;
    std::vector<AllocationEvent> allocation_history_;
    
    // Memory pools tracking
    std::unordered_map<std::string, MemoryPoolInfo> memory_pools_;
    
    std::atomic<size_t> total_allocated_bytes_{0};
    std::atomic<size_t> peak_allocated_bytes_{0};
    std::atomic<uint64_t> allocation_count_{0};
    std::atomic<uint64_t> deallocation_count_{0};
    
    #ifdef __EMSCRIPTEN__
    size_t wasm_heap_limit_ = 512 * 1024 * 1024; // 512MB default
    #endif
};

// Allocation information with call stack
struct AllocationInfo {
    size_t size;
    std::chrono::steady_clock::time_point timestamp;
    std::string file;
    int line;
    std::vector<void*> call_stack; // Platform-specific call stack collection
    
    [[nodiscard]] auto GetAge() const -> std::chrono::milliseconds {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - timestamp);
    }
};

// Memory allocation event for history tracking
struct AllocationEvent {
    enum class Type { Allocate, Deallocate, Reallocate };
    
    Type type;
    void* ptr;
    size_t size;
    std::chrono::steady_clock::time_point timestamp;
    std::string location; // file:line
    size_t total_memory_after; // Total allocated memory after this event
};

// Memory leak detection result
struct MemoryLeak {
    void* ptr;
    size_t size;
    std::chrono::milliseconds age;
    std::string allocation_location;
    std::vector<void*> call_stack;
    
    [[nodiscard]] auto GetSeverity() const -> LeakSeverity {
        if (age > std::chrono::minutes{10}) return LeakSeverity::High;
        if (age > std::chrono::minutes{1}) return LeakSeverity::Medium;
        return LeakSeverity::Low;
    }
};

// Memory statistics
struct MemoryStats {
    size_t current_allocated_bytes = 0;
    size_t peak_allocated_bytes = 0;
    uint64_t total_allocations = 0;
    uint64_t total_deallocations = 0;
    uint64_t active_allocations = 0;
    
    // Memory pool statistics
    std::unordered_map<std::string, MemoryPoolStats> pool_stats;
    
    // Platform-specific memory info
    size_t system_memory_total = 0;
    size_t system_memory_available = 0;
    
    #ifdef __EMSCRIPTEN__
    WebAssemblyMemoryStats wasm_stats;
    #endif
};

// WebAssembly-specific memory monitoring
struct WebAssemblyMemoryStats {
    size_t heap_size = 0;
    size_t heap_limit = 0;
    size_t heap_used = 0;
    float heap_utilization = 0.0f; // 0.0 to 1.0
    
    [[nodiscard]] auto IsNearLimit() const -> bool {
        return heap_utilization > 0.8f; // 80% threshold
    }
};

// Memory tracking macros with compile-time disabling
#ifdef MEMORY_PROFILING_ENABLED
    #define TRACK_ALLOCATION(ptr, size) \
        MemoryProfiler::Instance().RecordAllocation(ptr, size, __FILE__, __LINE__)
    #define TRACK_DEALLOCATION(ptr) \
        MemoryProfiler::Instance().RecordDeallocation(ptr)
    #define TRACK_REALLOCATION(old_ptr, new_ptr, size) \
        MemoryProfiler::Instance().RecordReallocation(old_ptr, new_ptr, size, __FILE__, __LINE__)
#else
    #define TRACK_ALLOCATION(ptr, size) ((void)0)
    #define TRACK_DEALLOCATION(ptr) ((void)0)
    #define TRACK_REALLOCATION(old_ptr, new_ptr, size) ((void)0)
#endif

} // namespace engine::profiling
```

### System Integration Profilers

#### ECS Performance Monitoring

```cpp
namespace engine::profiling {

// ECS-specific performance monitoring
class ECSProfiler {
public:
    explicit ECSProfiler(World& world) : world_(world) {}
    
    // Component and system monitoring
    void BeginSystemUpdate(const std::string& system_name);
    void EndSystemUpdate(const std::string& system_name);
    void RecordComponentQuery(const std::string& query_signature, size_t entity_count, std::chrono::microseconds duration);
    void RecordEntityOperation(EntityOperation operation, std::chrono::microseconds duration);
    
    // Data collection
    [[nodiscard]] auto GetSystemStats() const -> std::unordered_map<std::string, SystemPerformanceData>;
    [[nodiscard]] auto GetComponentStats() const -> std::unordered_map<std::string, ComponentPerformanceData>;
    [[nodiscard]] auto GetEntityStats() const -> EntityPerformanceData;
    
    // Frame-based analysis
    void BeginFrame();
    void EndFrame();
    [[nodiscard]] auto GetFrameStats() const -> ECSFrameStats;

private:
    World& world_;
    
    // System performance tracking
    std::unordered_map<std::string, SystemPerformanceData> system_stats_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> active_system_timers_;
    
    // Component query performance
    std::unordered_map<std::string, ComponentQueryStats> component_query_stats_;
    
    // Entity operation tracking
    EntityOperationStats entity_operation_stats_;
    
    mutable std::shared_mutex stats_mutex_;
};

// System performance data
struct SystemPerformanceData {
    std::chrono::microseconds total_time{0};
    std::chrono::microseconds average_time{0};
    std::chrono::microseconds min_time{std::chrono::microseconds::max()};
    std::chrono::microseconds max_time{0};
    uint32_t update_count = 0;
    uint32_t entities_processed = 0;
    
    // Threading information
    bool is_thread_safe = false;
    uint32_t parallel_executions = 0;
    
    [[nodiscard]] auto GetEntitiesPerSecond() const -> double {
        if (total_time.count() == 0) return 0.0;
        return static_cast<double>(entities_processed) / (static_cast<double>(total_time.count()) / 1000000.0);
    }
};

// Component query performance tracking
struct ComponentQueryStats {
    uint32_t query_count = 0;
    uint32_t total_entities_processed = 0;
    std::chrono::microseconds total_query_time{0};
    std::chrono::microseconds average_query_time{0};
    
    // Cache efficiency metrics
    uint32_t cache_hits = 0;
    uint32_t cache_misses = 0;
    
    [[nodiscard]] auto GetCacheHitRate() const -> float {
        uint32_t total_accesses = cache_hits + cache_misses;
        return total_accesses > 0 ? static_cast<float>(cache_hits) / static_cast<float>(total_accesses) : 0.0f;
    }
};

// Threading system performance monitoring
class ThreadingProfiler {
public:
    explicit ThreadingProfiler(JobSystem& job_system) : job_system_(job_system) {}
    
    // Job system monitoring
    void RecordJobSubmission(const std::string& job_name, JobPriority priority);
    void RecordJobExecution(const std::string& job_name, std::chrono::microseconds execution_time, uint32_t worker_id);
    void RecordJobCompletion(const std::string& job_name, std::chrono::microseconds total_time);
    
    // Worker thread monitoring
    void RecordWorkerIdle(uint32_t worker_id, std::chrono::microseconds idle_time);
    void RecordWorkerUtilization(uint32_t worker_id, float utilization_percentage);
    
    // Queue analysis
    void RecordQueueDepth(size_t queue_depth);
    void RecordWorkStealing(uint32_t thief_worker_id, uint32_t victim_worker_id);
    
    // Data collection
    [[nodiscard]] auto GetJobStats() const -> std::unordered_map<std::string, JobPerformanceData>;
    [[nodiscard]] auto GetWorkerStats() const -> std::vector<WorkerPerformanceData>;
    [[nodiscard]] auto GetQueueStats() const -> JobQueueStats;

private:
    JobSystem& job_system_;
    
    std::unordered_map<std::string, JobPerformanceData> job_stats_;
    std::vector<WorkerPerformanceData> worker_stats_;
    JobQueueStats queue_stats_;
    
    mutable std::shared_mutex stats_mutex_;
};

// Rendering system performance monitoring
class RenderingProfiler {
public:
    explicit RenderingProfiler(RenderSystem& render_system) : render_system_(render_system) {}
    
    // Draw call tracking
    void RecordDrawCall(const std::string& shader_name, uint32_t vertex_count, uint32_t instance_count);
    void RecordShaderSwitch(const std::string& from_shader, const std::string& to_shader);
    void RecordTextureBinding(uint32_t texture_id, uint32_t binding_slot);
    
    // GPU performance metrics
    void RecordFrameTime(std::chrono::microseconds cpu_time, std::chrono::microseconds gpu_time);
    void RecordGPUMemoryUsage(size_t used_bytes, size_t total_bytes);
    
    // Culling and optimization stats
    void RecordCullingStats(uint32_t total_objects, uint32_t culled_objects, uint32_t rendered_objects);
    void RecordBatchingStats(uint32_t individual_draws, uint32_t batched_draws, uint32_t saved_draws);
    
    [[nodiscard]] auto GetRenderingStats() const -> RenderingPerformanceData;

private:
    RenderSystem& render_system_;
    RenderingPerformanceData performance_data_;
    mutable std::shared_mutex stats_mutex_;
};

} // namespace engine::profiling
```

### Visualization and Development Interface

#### ImGui Profiler Dashboard

```cpp
namespace engine::profiling {

// Real-time profiler visualization using ImGui
class ProfilerUI {
public:
    explicit ProfilerUI(Profiler& profiler) : profiler_(profiler) {}
    
    // Main profiler window
    void RenderProfilerWindow();
    
    // Individual profiler panels
    void RenderFrameTimeGraph();
    void RenderSystemTimings();
    void RenderMemoryUsage();
    void RenderThreadingAnalysis();
    void RenderECSMetrics();
    void RenderRenderingStats();
    void RenderNetworkMetrics();
    
    // Configuration and controls
    void RenderProfilerControls();
    void RenderExportOptions();
    
    // Alert and notification system
    void RenderPerformanceAlerts();

private:
    Profiler& profiler_;
    
    // UI state
    bool show_profiler_window_ = false;
    bool show_frame_graph_ = true;
    bool show_system_timings_ = true;
    bool show_memory_usage_ = true;
    bool show_threading_analysis_ = false;
    bool show_ecs_metrics_ = false;
    bool show_rendering_stats_ = false;
    bool show_network_metrics_ = false;
    
    // Graph data
    std::array<float, 300> frame_time_history_;  // 5 seconds at 60 FPS
    size_t frame_time_history_index_ = 0;
    
    std::array<float, 300> memory_usage_history_;
    size_t memory_usage_history_index_ = 0;
    
    // Performance alerts
    std::vector<PerformanceAlert> active_alerts_;
    
    // UI helper methods
    void UpdateFrameTimeHistory();
    void UpdateMemoryUsageHistory();
    void CheckPerformanceAlerts();
    void RenderPerformanceGraph(const char* label, std::span<const float> data, float scale_min, float scale_max);
    void RenderSystemTable(const std::unordered_map<std::string, SystemPerformanceData>& system_stats);
};

// Performance alert system
struct PerformanceAlert {
    enum class Type { FrameTimeSpike, MemoryLeak, HighCPUUsage, GPUBottleneck, NetworkLatency };
    enum class Severity { Info, Warning, Critical };
    
    Type type;
    Severity severity;
    std::string message;
    std::chrono::steady_clock::time_point timestamp;
    bool is_acknowledged = false;
    
    [[nodiscard]] auto GetAge() const -> std::chrono::seconds {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - timestamp);
    }
};

// ImGui integration with profiler
void ProfilerUI::RenderProfilerWindow() {
    if (!show_profiler_window_) return;
    
    ImGui::Begin("Engine Profiler", &show_profiler_window_, ImGuiWindowFlags_MenuBar);
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Frame Time Graph", nullptr, &show_frame_graph_);
            ImGui::MenuItem("System Timings", nullptr, &show_system_timings_);
            ImGui::MenuItem("Memory Usage", nullptr, &show_memory_usage_);
            ImGui::MenuItem("Threading Analysis", nullptr, &show_threading_analysis_);
            ImGui::MenuItem("ECS Metrics", nullptr, &show_ecs_metrics_);
            ImGui::MenuItem("Rendering Stats", nullptr, &show_rendering_stats_);
            ImGui::MenuItem("Network Metrics", nullptr, &show_network_metrics_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Export Data")) {
                // Trigger data export
                profiler_.ExportData("profiler_data.json", ExportFormat::JSON);
            }
            if (ImGui::MenuItem("Clear Data")) {
                profiler_.ClearData();
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    // Profiler controls
    RenderProfilerControls();
    
    ImGui::Separator();
    
    // Performance alerts
    RenderPerformanceAlerts();
    
    // Tabbed interface for different profiler views
    if (ImGui::BeginTabBar("ProfilerTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            if (show_frame_graph_) RenderFrameTimeGraph();
            if (show_system_timings_) RenderSystemTimings();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Memory")) {
            if (show_memory_usage_) RenderMemoryUsage();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Threading")) {
            if (show_threading_analysis_) RenderThreadingAnalysis();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("ECS")) {
            if (show_ecs_metrics_) RenderECSMetrics();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Rendering")) {
            if (show_rendering_stats_) RenderRenderingStats();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Network")) {
            if (show_network_metrics_) RenderNetworkMetrics();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void ProfilerUI::RenderFrameTimeGraph() {
    ImGui::Text("Frame Time Analysis");
    
    // Update frame time history
    UpdateFrameTimeHistory();
    
    // Frame time graph
    RenderPerformanceGraph("Frame Time (ms)", frame_time_history_, 0.0f, 33.33f); // 0-33ms (30-60 FPS)
    
    // Frame time statistics
    auto frame_data = profiler_.GetCurrentFrameData();
    ImGui::Text("Current Frame: %.2f ms", frame_data.frame_duration.count() / 1000.0f);
    ImGui::Text("Target: 16.67 ms (60 FPS)");
    
    // FPS calculation
    float current_fps = frame_data.frame_duration.count() > 0 ? 
        1000000.0f / static_cast<float>(frame_data.frame_duration.count()) : 0.0f;
    ImGui::Text("Current FPS: %.1f", current_fps);
    
    // Performance alerts for frame time spikes
    if (frame_data.frame_duration.count() > 33333) { // > 33.33ms (< 30 FPS)
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Frame time spike detected!");
    }
}

void ProfilerUI::RenderSystemTimings() {
    ImGui::Text("System Performance");
    
    auto system_stats = profiler_.GetSystemStats();
    
    if (ImGui::BeginTable("SystemStats", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("System");
        ImGui::TableSetupColumn("Avg Time (μs)");
        ImGui::TableSetupColumn("Max Time (μs)");
        ImGui::TableSetupColumn("Calls");
        ImGui::TableSetupColumn("% of Frame");
        ImGui::TableHeadersRow();
        
        auto frame_data = profiler_.GetCurrentFrameData();
        auto frame_time_us = frame_data.frame_duration.count();
        
        for (const auto& [system_name, section_data] : frame_data.sections) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", system_name.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", section_data.GetAverageTime().count());
            
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", section_data.max_time.count());
            
            ImGui::TableNextColumn();
            ImGui::Text("%u", section_data.call_count);
            
            ImGui::TableNextColumn();
            float percentage = frame_time_us > 0 ? 
                (static_cast<float>(section_data.total_time.count()) / static_cast<float>(frame_time_us)) * 100.0f : 0.0f;
            ImGui::Text("%.1f%%", percentage);
        }
        
        ImGui::EndTable();
    }
}

} // namespace engine::profiling
```

### Scripting Integration

#### Profiler Script API

```cpp
namespace engine::scripting {

// Profiler API for scripting languages
class ScriptProfilerAPI {
public:
    explicit ScriptProfilerAPI(profiling::Profiler& profiler) : profiler_(profiler) {}
    
    // Profiler control
    void EnableProfiling(bool enabled);
    [[nodiscard]] auto IsProfilingEnabled() const -> bool;
    
    // Custom profiling scopes
    void BeginProfileScope(const std::string& name);
    void EndProfileScope(const std::string& name);
    
    // Custom metrics
    void RecordCounter(const std::string& name, int64_t value);
    void RecordGauge(const std::string& name, double value);
    void RecordTiming(const std::string& name, double milliseconds);
    
    // Performance queries
    [[nodiscard]] auto GetFrameTime() const -> double;
    [[nodiscard]] auto GetSystemTime(const std::string& system_name) const -> double;
    [[nodiscard]] auto GetMemoryUsage() const -> ScriptValue;
    [[nodiscard]] auto GetFPS() const -> double;
    
    // Performance alerts
    void SetPerformanceAlert(const std::string& metric_name, double threshold, const std::string& callback_function);
    void ClearPerformanceAlert(const std::string& metric_name);
    
    // Data export
    void ExportProfileData(const std::string& file_path, const std::string& format = "json");
    void ClearProfileData();

private:
    profiling::Profiler& profiler_;
    std::unordered_map<std::string, std::function<void()>> alert_callbacks_;
};

// Integration with unified scripting interface
void RegisterProfilerAPI(ScriptInterface& script_interface, profiling::Profiler& profiler) {
    auto profiler_api = std::make_unique<ScriptProfilerAPI>(profiler);
    script_interface.RegisterAPI("profiler", std::move(profiler_api));
}

} // namespace engine::scripting

// Example Lua integration
/*
-- Lua script example for custom profiling
function on_game_update()
    profiler.begin_profile_scope("custom_game_logic")
    
    -- Custom game logic here
    process_ai_decisions()
    update_resource_management()
    
    profiler.end_profile_scope("custom_game_logic")
    
    -- Record custom metrics
    profiler.record_counter("active_entities", get_entity_count())
    profiler.record_gauge("player_satisfaction", calculate_satisfaction())
end

function setup_performance_monitoring()
    -- Set up performance alerts
    profiler.set_performance_alert("frame_time", 33.33, "on_frame_time_spike")
    profiler.set_performance_alert("memory_usage", 400000000, "on_memory_usage_high") -- 400MB
    
    -- Enable profiling for development
    profiler.enable_profiling(true)
end

function on_frame_time_spike()
    print("Warning: Frame time exceeded 33.33ms!")
    -- Could trigger performance optimizations or quality reductions
end

function on_memory_usage_high()
    print("Warning: Memory usage exceeded 400MB!")
    -- Could trigger garbage collection or asset unloading
end

function export_performance_data()
    profiler.export_profile_data("performance_session.json")
    print("Performance data exported for analysis")
end
*/
```

## Performance Requirements

### Target Specifications

- **Zero Overhead**: Profiling system adds <0.1% CPU overhead when disabled in release builds
- **Development Overhead**: <5% CPU overhead when enabled in development builds with full profiling
- **Memory Usage**: Profiling system uses <20MB memory for data collection and visualization
- **Real-Time Response**: Profiler UI updates at 60+ FPS without impacting game performance
- **Data Collection**: Support 1000+ profile scopes with microsecond precision timing
- **Cross-Platform Performance**: Identical profiling capabilities and overhead across all platforms

### Optimization Strategies

1. **Compile-Time Elimination**: Profile macros completely removed in release builds via preprocessor
2. **Lock-Free Collection**: Thread-local data collection with minimal synchronization overhead
3. **Efficient Storage**: Ring buffers and memory pools for profile data to minimize allocations
4. **Adaptive Sampling**: Reduce profiling frequency under heavy load to maintain performance
5. **Lazy Visualization**: UI updates only when profiler window is visible

## Integration Points

### ECS Core Integration

- Profiling hooks integrated into system execution pipeline
- Component query performance monitoring with cache efficiency metrics
- Entity operation timing for creation, destruction, and component manipulation

### Threading System Integration

- Job system performance monitoring with worker thread utilization tracking
- Work-stealing efficiency analysis and queue depth monitoring
- Thread-safe profile data collection compatible with parallel ECS execution

### All Engine Systems Integration

- Physics system timing and collision detection performance
- Rendering system draw call tracking and GPU performance metrics
- Audio system performance monitoring and spatial audio processing times
- Networking system bandwidth and latency monitoring
- Asset system loading performance and hot-reload timing analysis

### Platform Abstraction Integration

- High-resolution timing using platform-specific APIs for maximum precision
- Memory usage tracking using platform memory management APIs
- Cross-platform profiling data export and configuration management

### Scripting System Integration

- Complete profiler API exposed to Lua, Python, and AngelScript
- Custom performance metrics definable through scripts
- Performance alert system with script callback integration

This profiling system provides comprehensive development and optimization tools while maintaining zero overhead in
production builds. The real-time visualization and cross-platform compatibility ensure consistent development workflows
across all target platforms, completing your engine's development infrastructure.
