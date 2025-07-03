# SYS_THREADING_v1

> **System-Level Design Document for Threading and Job System**

## Executive Summary

This document defines a modern C++20 threading system that enables efficient parallel execution of ECS systems within
the established frame pipelining architecture. The design emphasizes lock-free data structures, automatic dependency
resolution, and seamless integration with the existing main thread (events) + render thread (game logic + rendering) +
I/O thread pool model. The system supports both high-performance native execution and graceful single-threaded fallback
for WebAssembly builds, while maintaining deterministic behavior critical for save/load consistency.

## Scope and Objectives

### In Scope

- [ ] Job system with work-stealing thread pool for ECS system execution
- [ ] Lock-free component access patterns using atomic operations and memory ordering
- [ ] Automatic dependency resolution based on component read/write declarations
- [ ] Frame pipelining coordination between main thread, render thread, and system thread pool
- [ ] C++20 coroutine integration for asynchronous task composition
- [ ] WebAssembly single-threaded execution fallback with identical API
- [ ] Performance profiling hooks for job execution timing and thread utilization
- [ ] Memory ordering guarantees for cross-system communication
- [ ] Thread-safe resource management for GPU assets and game data
- [ ] Platform abstraction integration for threading primitives and CPU detection

### Out of Scope

- [ ] Networking thread management (future extension)
- [ ] Audio thread coordination (handled by OpenAL)
- [ ] File I/O thread pool implementation (already exists in architecture)
- [ ] Low-level platform threading APIs (handled by Platform Abstraction system)
- [ ] Platform-specific thread affinity optimization (Phase 2)
- [ ] Real-time thread scheduling policies (gaming workloads don't require RT)
- [ ] Distributed computing or inter-process communication

### Primary Objectives

1. **Performance**: Execute 50+ ECS systems in parallel with automatic load balancing
2. **Determinism**: Ensure consistent execution order and results across runs and platforms
3. **Integration**: Seamless integration with existing threading model and OpenGL context management
4. **Scalability**: Efficient utilization of 2-16 CPU cores with linear performance scaling
5. **WebAssembly Compatibility**: Identical API with single-threaded execution for web builds

### Secondary Objectives

- Future extensibility for GPU compute integration (OpenGL compute shaders)
- Comprehensive debugging and profiling tools for thread performance analysis
- Memory-efficient operation suitable for WebAssembly heap constraints
- Hot-reload support for system dependency changes during development
- Integration points for external profilers (Tracy, Intel VTune, etc.)

## Architecture/Design

### High-Level Overview

```
Existing Threading Model Integration:
┌───────────────┬───────────────────────────────┬─────────────┐
│ Main Thread   │          Render Thread        │   I/O       │
│ (Events &     │         (Sequential)          │  Thread     │
│ Coordination) │                               │  Pool       │
│               │  ┌─────────────┐              │             │
│               │  │ System      │◄─────────────┤             │
│               │  │ Thread Pool │              │             │
│               │  │ (NEW)       │              │             │
│               │  └─────────────┘              │             │
│               │         ↓                     │             │
│               │  ┌─────────────┐              │             │
│               │  │   OpenGL    │              │             │
│               │  │ Rendering   │              │             │
│               │  └─────────────┘              │             │
└───────────────┴───────────────────────────────┴─────────────┘

Job System Architecture:
┌─────────────────────────────────────────────────────────────────┐
│                    System Scheduler                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Dependency  │ │ Execution   │ │ Synchron-   │               │
│  │ Resolution  │ │ Batching    │ │ ization     │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Job System Core                              │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Work Queue  │ │ Thread Pool │ │ Job         │               │
│  │ (Lock-Free) │ │ Management  │ │ Execution   │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Component Access Layer                             │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Atomic      │ │ Memory      │ │ Lock-Free   │               │
│  │ Operations  │ │ Ordering    │ │ Iteration   │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Job System Core

- **Purpose**: Work distribution, thread management, and execution coordination
- **Responsibilities**: Job scheduling, work stealing, thread lifecycle, load balancing
- **Key Classes/Interfaces**: `JobSystem`, `WorkStealingQueue`, `ThreadPool`, `Job`
- **Data Flow**: System jobs → Queue submission → Work stealing → Parallel execution

#### Component 2: Dependency Resolution Engine

- **Purpose**: Automatic system ordering based on component access patterns
- **Responsibilities**: Build dependency graph, detect cycles, generate execution batches
- **Key Classes/Interfaces**: `DependencyGraph`, `SystemMetadata`, `ExecutionBatch`
- **Data Flow**: System registration → Dependency analysis → Batch generation → Execution

#### Component 3: Lock-Free Component Access

- **Purpose**: Thread-safe component iteration and modification
- **Responsibilities**: Atomic component operations, memory ordering, version tracking
- **Key Classes/Interfaces**: `ComponentAccessor`, `AtomicComponentArray`, `VersionTracker`
- **Data Flow**: System queries → Lock-free iteration → Atomic updates → Memory synchronization

#### Component 4: Frame Synchronization Coordinator

- **Purpose**: Integration with existing threading model and frame pipelining
- **Responsibilities**: Render thread coordination, I/O synchronization, frame barriers
- **Key Classes/Interfaces**: `FrameCoordinator`, `SynchronizationPoint`, `FrameBarrier`
- **Data Flow**: Frame start → System execution → Synchronization → Render submission

### Job System Implementation

#### Core Job Abstraction

```cpp
// Modern C++20 job definition with concepts
template<typename Callable>
concept JobFunction = std::invocable<Callable> && 
                     std::same_as<void, std::invoke_result_t<Callable>>;

class Job {
public:
    template<JobFunction F>
    explicit Job(F&& function, std::string_view name = "Anonymous")
        : function_(std::forward<F>(function))
        , name_(name)
        , creation_time_(std::chrono::high_resolution_clock::now()) {}
    
    void Execute() noexcept {
        try {
            function_();
            state_.store(JobState::Completed, std::memory_order_release);
        } catch (...) {
            state_.store(JobState::Failed, std::memory_order_release);
            // Log exception details
        }
    }
    
    [[nodiscard]] auto IsComplete() const noexcept -> bool {
        return state_.load(std::memory_order_acquire) == JobState::Completed;
    }
    
    [[nodiscard]] auto GetName() const noexcept -> std::string_view { return name_; }
    
private:
    enum class JobState : uint8_t { Pending, Running, Completed, Failed };
    
    std::function<void()> function_;
    std::string name_;
    std::atomic<JobState> state_{JobState::Pending};
    std::chrono::high_resolution_clock::time_point creation_time_;
};
```

#### Work-Stealing Thread Pool

```cpp
// High-performance work-stealing implementation
class WorkStealingQueue {
public:
    void Push(std::unique_ptr<Job> job) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        jobs_[tail & mask_] = std::move(job);
        tail_.store(tail + 1, std::memory_order_release);
    }
    
    [[nodiscard]] auto Pop() -> std::unique_ptr<Job> {
        const auto tail = tail_.load(std::memory_order_relaxed) - 1;
        tail_.store(tail, std::memory_order_relaxed);
        
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        const auto head = head_.load(std::memory_order_relaxed);
        if (tail < head) {
            tail_.store(head, std::memory_order_relaxed);
            return nullptr;
        }
        
        auto job = std::move(jobs_[tail & mask_]);
        if (tail > head) {
            return job;
        }
        
        // Potential race with steal - use CAS to resolve
        tail_.store(head + 1, std::memory_order_relaxed);
        return head_.compare_exchange_weak(head, head + 1, 
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed) ? job : nullptr;
    }
    
    [[nodiscard]] auto Steal() -> std::unique_ptr<Job> {
        const auto head = head_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const auto tail = tail_.load(std::memory_order_relaxed);
        
        if (head >= tail) {
            return nullptr;
        }
        
        auto job = std::move(jobs_[head & mask_]);
        return head_.compare_exchange_weak(head, head + 1,
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed) ? job : nullptr;
    }
    
private:
    static constexpr size_t queue_size = 4096;
    static constexpr size_t mask_ = queue_size - 1;
    
    std::array<std::unique_ptr<Job>, queue_size> jobs_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

class JobSystem {
public:
    explicit JobSystem(size_t thread_count = 0) 
        : thread_count_(thread_count == 0 ? std::thread::hardware_concurrency() : thread_count)
        , worker_queues_(thread_count_)
        , workers_(thread_count_) {
        StartWorkers();
    }
    
    ~JobSystem() {
        StopWorkers();
    }
    
    template<JobFunction F>
    void Submit(F&& function, std::string_view name = "Anonymous") {
        auto job = std::make_unique<Job>(std::forward<F>(function), name);
        
        // Submit to current thread's queue, or distribute round-robin
        const auto thread_id = GetCurrentWorkerIndex();
        worker_queues_[thread_id].Push(std::move(job));
        
        // Wake a sleeping worker
        cv_.notify_one();
    }
    
    void WaitForCompletion() {
        // Help with work while waiting
        while (HasPendingJobs()) {
            if (auto job = StealWork()) {
                job->Execute();
            } else {
                std::this_thread::yield();
            }
        }
    }
    
private:
    size_t thread_count_;
    std::vector<WorkStealingQueue> worker_queues_;
    std::vector<std::jthread> workers_;
    std::condition_variable cv_;
    std::mutex cv_mutex_;
    std::atomic<bool> should_stop_{false};
    
    void WorkerFunction(std::stop_token stop_token, size_t worker_id);
    [[nodiscard]] auto StealWork() -> std::unique_ptr<Job>;
    [[nodiscard]] auto HasPendingJobs() const -> bool;
    [[nodiscard]] auto GetCurrentWorkerIndex() const -> size_t;
};
```

### Dependency Resolution System

#### System Metadata and Registration

```cpp
// C++20 concepts for system requirements
template<typename T>
concept ComponentType = std::is_trivially_copyable_v<T> && 
                       std::is_default_constructible_v<T>;

template<typename T>
concept SystemType = requires(T system, World& world, float delta_time) {
    system.Update(world, delta_time);
    { system.GetThreadingRequirements() } -> std::same_as<ThreadingRequirements>;
};

struct ThreadingRequirements {
    std::vector<ComponentTypeId> read_components;
    std::vector<ComponentTypeId> write_components;
    ExecutionPhase phase = ExecutionPhase::Update;
    ThreadSafety safety = ThreadSafety::ThreadSafe;
    std::vector<std::string> system_dependencies; // Optional explicit dependencies
};

enum class ThreadSafety : uint8_t {
    ThreadSafe,    // Can run in parallel with other thread-safe systems
    MainThreadOnly // Must execute on main (render) thread
};

// System registration with automatic dependency detection
class SystemScheduler {
public:
    template<SystemType T>
    void RegisterSystem(std::unique_ptr<T> system) {
        auto requirements = system->GetThreadingRequirements();
        systems_.emplace_back(std::move(system), std::move(requirements));
        dependency_graph_dirty_ = true;
    }
    
    void ExecuteFrame(World& world, float delta_time) {
        if (dependency_graph_dirty_) {
            RebuildDependencyGraph();
        }
        
        for (const auto& batch : execution_batches_) {
            ExecuteBatch(batch, world, delta_time);
        }
    }
    
private:
    struct SystemData {
        std::unique_ptr<ISystem> system;
        ThreadingRequirements requirements;
        SystemId id;
    };
    
    std::vector<SystemData> systems_;
    std::vector<ExecutionBatch> execution_batches_;
    bool dependency_graph_dirty_ = true;
    
    void RebuildDependencyGraph();
    void ExecuteBatch(const ExecutionBatch& batch, World& world, float delta_time);
};
```

#### Dependency Graph Construction

```cpp
// Automatic dependency resolution based on component access
class DependencyGraph {
public:
    void AddSystem(SystemId id, const ThreadingRequirements& requirements) {
        nodes_[id] = DependencyNode{
            .id = id,
            .requirements = requirements,
            .dependencies = {},
            .dependents = {}
        };
        
        // Build edges based on component access conflicts
        for (const auto& [other_id, other_node] : nodes_) {
            if (other_id == id) continue;
            
            if (HasAccessConflict(requirements, other_node.requirements)) {
                AddDependency(other_id, id); // other_id must execute before id
            }
        }
    }
    
    [[nodiscard]] auto GenerateExecutionBatches() const -> std::vector<ExecutionBatch> {
        std::vector<ExecutionBatch> batches;
        std::unordered_set<SystemId> completed;
        
        while (completed.size() < nodes_.size()) {
            ExecutionBatch batch;
            
            // Find all systems ready to execute (dependencies satisfied)
            for (const auto& [id, node] : nodes_) {
                if (completed.contains(id)) continue;
                
                bool dependencies_satisfied = std::ranges::all_of(
                    node.dependencies, 
                    [&completed](SystemId dep) { return completed.contains(dep); }
                );
                
                if (dependencies_satisfied) {
                    batch.systems.push_back(id);
                }
            }
            
            if (batch.systems.empty()) {
                throw std::runtime_error("Circular dependency detected in system graph");
            }
            
            batches.push_back(std::move(batch));
            
            // Mark batch systems as completed
            for (SystemId id : batch.systems) {
                completed.insert(id);
            }
        }
        
        return batches;
    }
    
private:
    struct DependencyNode {
        SystemId id;
        ThreadingRequirements requirements;
        std::vector<SystemId> dependencies;
        std::vector<SystemId> dependents;
    };
    
    std::unordered_map<SystemId, DependencyNode> nodes_;
    
    [[nodiscard]] auto HasAccessConflict(const ThreadingRequirements& a, 
                                       const ThreadingRequirements& b) const -> bool {
        // Write-write conflicts
        for (ComponentTypeId comp_a : a.write_components) {
            if (std::ranges::find(b.write_components, comp_a) != b.write_components.end()) {
                return true;
            }
        }
        
        // Read-write conflicts
        for (ComponentTypeId comp_a : a.write_components) {
            if (std::ranges::find(b.read_components, comp_a) != b.read_components.end()) {
                return true;
            }
        }
        
        for (ComponentTypeId comp_a : a.read_components) {
            if (std::ranges::find(b.write_components, comp_a) != b.write_components.end()) {
                return true;
            }
        }
        
        return false;
    }
    
    void AddDependency(SystemId from, SystemId to) {
        nodes_[from].dependents.push_back(to);
        nodes_[to].dependencies.push_back(from);
    }
};
```

### Lock-Free Component Access

#### Atomic Component Operations

```cpp
// Thread-safe component access with memory ordering
template<ComponentType T>
class AtomicComponentArray {
public:
    explicit AtomicComponentArray(size_t max_entities) 
        : components_(max_entities)
        , entity_versions_(max_entities)
        , generation_counter_(1) {}
    
    // Thread-safe component access
    [[nodiscard]] auto Get(EntityId entity) const -> std::optional<T> {
        const auto index = GetEntityIndex(entity);
        if (index >= components_.size()) {
            return std::nullopt;
        }
        
        // Load with acquire semantics for proper ordering
        const auto version = entity_versions_[index].load(std::memory_order_acquire);
        if (version == 0) {
            return std::nullopt; // Component doesn't exist
        }
        
        return components_[index];
    }
    
    void Set(EntityId entity, const T& component) {
        const auto index = GetEntityIndex(entity);
        if (index >= components_.size()) {
            return; // Entity out of bounds
        }
        
        components_[index] = component;
        
        // Update version with release semantics
        const auto new_version = generation_counter_.fetch_add(1, std::memory_order_relaxed);
        entity_versions_[index].store(new_version, std::memory_order_release);
    }
    
    void Remove(EntityId entity) {
        const auto index = GetEntityIndex(entity);
        if (index >= components_.size()) {
            return;
        }
        
        // Mark as removed with release semantics
        entity_versions_[index].store(0, std::memory_order_release);
    }
    
    // Lock-free iteration for systems
    template<typename Func>
    void ForEach(Func&& func) const requires std::invocable<Func, EntityId, const T&> {
        for (size_t i = 0; i < components_.size(); ++i) {
            const auto version = entity_versions_[i].load(std::memory_order_acquire);
            if (version != 0) {
                const auto entity = GetEntityFromIndex(i);
                func(entity, components_[i]);
            }
        }
    }
    
private:
    std::vector<T> components_;
    std::vector<std::atomic<uint64_t>> entity_versions_;
    std::atomic<uint64_t> generation_counter_;
    
    [[nodiscard]] auto GetEntityIndex(EntityId entity) const -> size_t {
        return entity & 0xFFFFFF; // Lower 24 bits for index
    }
    
    [[nodiscard]] auto GetEntityFromIndex(size_t index) const -> EntityId {
        return static_cast<EntityId>(index); // Simplified for this example
    }
};
```

### C++20 Coroutine Integration

#### Asynchronous Task Composition

```cpp
// Coroutine support for complex async operations
template<typename T>
class Task {
public:
    struct promise_type {
        auto initial_suspend() { return std::suspend_never{}; }
        auto final_suspend() noexcept { return std::suspend_never{}; }
        auto get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        auto unhandled_exception() { std::terminate(); }
        
        template<typename U>
        auto return_value(U&& value) {
            result_ = std::forward<U>(value);
        }
        
        T result_;
    };
    
    explicit Task(std::coroutine_handle<promise_type> coro) : coro_(coro) {}
    
    ~Task() {
        if (coro_) {
            coro_.destroy();
        }
    }
    
    [[nodiscard]] auto GetResult() const -> const T& {
        return coro_.promise().result_;
    }
    
private:
    std::coroutine_handle<promise_type> coro_;
};

// Async system execution with coroutines
class AsyncSystemExecutor {
public:
    template<SystemType T>
    auto ExecuteSystemAsync(T& system, World& world, float delta_time) -> Task<void> {
        // Pre-execution setup
        co_await ScheduleJob([&]() {
            PrepareSystemExecution(system);
        });
        
        // Main system execution
        co_await ScheduleJob([&]() {
            system.Update(world, delta_time);
        });
        
        // Post-execution cleanup
        co_await ScheduleJob([&]() {
            FinalizeSystemExecution(system);
        });
        
        co_return;
    }
    
private:
    auto ScheduleJob(std::function<void()> job) -> Task<void> {
        // Submit job to thread pool and wait for completion
        job_system_.Submit(std::move(job));
        co_return;
    }
    
    JobSystem& job_system_;
};
```

### WebAssembly Fallback Implementation

#### Single-Threaded Execution Mode

```cpp
// Conditional compilation for WebAssembly
#ifdef __EMSCRIPTEN__
    constexpr bool is_webassembly_build = true;
#else
    constexpr bool is_webassembly_build = false;
#endif

class PlatformJobSystem {
public:
    explicit PlatformJobSystem(size_t thread_count = 0) {
        if constexpr (is_webassembly_build) {
            // WebAssembly: single-threaded execution
            impl_ = std::make_unique<SingleThreadedJobSystem>();
        } else {
            // Native: multi-threaded execution
            impl_ = std::make_unique<MultiThreadedJobSystem>(thread_count);
        }
    }
    
    template<JobFunction F>
    void Submit(F&& function, std::string_view name = "Anonymous") {
        impl_->Submit(std::forward<F>(function), name);
    }
    
    void WaitForCompletion() {
        impl_->WaitForCompletion();
    }
    
private:
    class JobSystemInterface {
    public:
        virtual ~JobSystemInterface() = default;
        virtual void Submit(std::function<void()> job, std::string_view name) = 0;
        virtual void WaitForCompletion() = 0;
    };
    
    class SingleThreadedJobSystem final : public JobSystemInterface {
    public:
        void Submit(std::function<void()> job, std::string_view name) override {
            pending_jobs_.emplace_back(std::move(job), name);
        }
        
        void WaitForCompletion() override {
            // Execute all jobs immediately on main thread
            for (auto& [job, name] : pending_jobs_) {
                job();
            }
            pending_jobs_.clear();
        }
        
    private:
        std::vector<std::pair<std::function<void()>, std::string>> pending_jobs_;
    };
    
    using MultiThreadedJobSystem = JobSystem; // The full implementation from above
    
    std::unique_ptr<JobSystemInterface> impl_;
};
```

### Frame Synchronization Integration

#### Coordination with Existing Threading Model

```cpp
// Integration with the established render thread architecture
class FrameCoordinator {
public:
    // Called from render thread at frame start
    void BeginFrame() {
        frame_number_.fetch_add(1, std::memory_order_relaxed);
        
        // Signal I/O thread pool that new frame has started
        io_sync_point_.SignalFrameStart(frame_number_.load());
        
        // Reset system execution barriers
        system_barrier_.Reset();
    }
    
    // Called from render thread before ECS system execution
    void BeginSystemExecution(World& world, float delta_time) {
        // Execute all systems in dependency-resolved batches
        system_scheduler_.ExecuteFrame(world, delta_time);
        
        // Wait for all system jobs to complete before rendering
        job_system_.WaitForCompletion();
    }
    
    // Called from render thread before OpenGL rendering
    void BeginRenderPhase() {
        // Ensure all ECS systems have completed
        system_barrier_.Wait();
        
        // OpenGL context is available on this thread - proceed with rendering
    }
    
    // Called from render thread at frame end
    void EndFrame() {
        // Synchronize with I/O operations if needed
        io_sync_point_.WaitForFrameCompletion();
        
        // Update performance metrics
        UpdateFrameMetrics();
    }
    
private:
    std::atomic<uint64_t> frame_number_{0};
    SystemScheduler& system_scheduler_;
    JobSystem& job_system_;
    FrameBarrier system_barrier_;
    IOSynchronizationPoint io_sync_point_;
    
    void UpdateFrameMetrics();
};

// Barrier for synchronizing system execution phases
class FrameBarrier {
public:
    void Reset() {
        std::unique_lock lock(mutex_);
        waiting_count_ = 0;
        signaled_ = false;
    }
    
    void Wait() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return signaled_; });
    }
    
    void Signal() {
        std::unique_lock lock(mutex_);
        signaled_ = true;
        cv_.notify_all();
    }
    
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<size_t> waiting_count_{0};
    bool signaled_ = false;
};
```

## Performance Optimization Strategies

### Work Distribution and Load Balancing

```cpp
// Intelligent work distribution based on system complexity
class WorkloadAnalyzer {
public:
    struct SystemProfile {
        std::chrono::microseconds avg_execution_time{0};
        size_t entity_count = 0;
        float complexity_score = 1.0f;
        ExecutionPhase phase = ExecutionPhase::Update;
    };
    
    void ProfileSystem(SystemId id, std::chrono::microseconds execution_time, size_t entities) {
        auto& profile = system_profiles_[id];
        
        // Exponential moving average for execution time
        constexpr float alpha = 0.1f;
        profile.avg_execution_time = std::chrono::microseconds{
            static_cast<int64_t>(alpha * execution_time.count() + 
                                (1.0f - alpha) * profile.avg_execution_time.count())
        };
        
        profile.entity_count = entities;
        profile.complexity_score = CalculateComplexityScore(execution_time, entities);
    }
    
    [[nodiscard]] auto OptimizeBatchDistribution(const std::vector<ExecutionBatch>& batches) 
        -> std::vector<ExecutionBatch> {
        std::vector<ExecutionBatch> optimized_batches;
        
        for (const auto& batch : batches) {
            // Split large batches if they exceed optimal size
            if (ShouldSplitBatch(batch)) {
                auto split_batches = SplitBatch(batch);
                optimized_batches.insert(optimized_batches.end(), 
                                       split_batches.begin(), split_batches.end());
            } else {
                optimized_batches.push_back(batch);
            }
        }
        
        return optimized_batches;
    }
    
private:
    std::unordered_map<SystemId, SystemProfile> system_profiles_;
    
    [[nodiscard]] auto CalculateComplexityScore(std::chrono::microseconds time, size_t entities) const -> float {
        if (entities == 0) return 1.0f;
        return static_cast<float>(time.count()) / static_cast<float>(entities);
    }
    
    [[nodiscard]] auto ShouldSplitBatch(const ExecutionBatch& batch) const -> bool;
    [[nodiscard]] auto SplitBatch(const ExecutionBatch& batch) const -> std::vector<ExecutionBatch>;
};
```

## Success Criteria

### Performance Metrics

1. **System Execution**: 50+ systems executing in parallel with <2ms overhead per frame
2. **Thread Utilization**: >85% CPU utilization on systems with 4+ cores during heavy workloads
3. **Dependency Resolution**: Automatic dependency graph generation in <1ms for 100+ systems
4. **WebAssembly Parity**: Single-threaded fallback with <15% performance degradation vs. native

### Threading Safety

1. **Data Race Freedom**: Zero data races detected during 24-hour stress testing with TSan
2. **Memory Ordering**: Correct acquire-release semantics for all cross-thread communication
3. **Deterministic Execution**: Identical results across multiple runs with same input
4. **Deadlock Prevention**: No deadlocks possible due to lock-free design

### Integration Requirements

1. **OpenGL Context Safety**: All rendering operations remain on render thread
2. **Frame Pipelining**: Smooth integration with existing main/render/I/O thread model
3. **Hot-Reload Support**: System dependency changes applied without engine restart
4. **Profiling Integration**: Performance data available to external profiling tools

## Future Enhancements

### Phase 2: Advanced Threading Features

#### GPU Compute Integration

```cpp
// OpenGL compute shader integration for parallel data processing
class ComputeJobSystem {
public:
    template<typename T>
    auto SubmitComputeJob(std::span<T> data, std::string_view shader_name) -> ComputeJob<T> {
        // Execute data processing on GPU using compute shaders
        // Useful for large-scale physics, AI pathfinding, or data transformations
    }
    
private:
    // Coordinate between CPU job system and GPU compute execution
    void SynchronizeCPUGPUWorkloads();
};
```

#### Advanced Scheduling Policies

```cpp
// Priority-based job scheduling for time-critical systems
enum class JobPriority : uint8_t {
    Low,        // Background systems (analytics, logging)
    Normal,     // Standard game systems
    High,       // Player input, critical AI
    RealTime    // Audio, network synchronization
};

class PriorityJobSystem {
public:
    template<JobFunction F>
    void Submit(F&& function, JobPriority priority = JobPriority::Normal);
    
private:
    // Multiple priority queues with preemption support
    std::array<WorkStealingQueue, 4> priority_queues_;
};
```

#### Memory Pool Integration

```cpp
// Thread-local memory pools for allocation-heavy systems
class ThreadLocalAllocator {
public:
    template<typename T>
    [[nodiscard]] auto Allocate(size_t count = 1) -> T* {
        // Fast thread-local allocation without contention
        // Automatically freed at frame end
    }
    
    void ResetFrame() {
        // Bulk free all frame allocations
    }
};
```

### Phase 3: Distributed Computing Support

#### Network-Aware Job Distribution

```cpp
// Future extension for distributed simulation across multiple machines
class DistributedJobSystem {
public:
    void RegisterRemoteWorker(const NetworkAddress& address);
    void SubmitRemoteJob(std::function<void()> job, const NetworkAddress& target);
    
private:
    // Coordinate job distribution across network
    // Useful for large-scale simulations or dedicated servers
};
```

#### Persistent Threading Profiles

```cpp
// Machine learning-based system optimization
class AdaptiveScheduler {
public:
    void LoadPerformanceProfile(const std::filesystem::path& profile_path);
    void SavePerformanceProfile(const std::filesystem::path& profile_path);
    
private:
    // Learn optimal system scheduling based on historical performance
    // Adapt to different hardware configurations automatically
};
```

### Phase 4: Real-Time Extensions

#### Soft Real-Time Guarantees

```cpp
// Frame time budgets with graceful degradation
class TimeBudgetedScheduler {
public:
    void SetFrameBudget(std::chrono::microseconds budget);
    void SetSystemPriority(SystemId id, float priority);
    
private:
    // Ensure critical systems complete within frame budget
    // Skip or defer less important systems if time is short
};
```

## Risk Mitigation

### Threading Complexity Risks

- **Risk**: Difficult-to-debug threading issues (race conditions, deadlocks)
- **Mitigation**: Lock-free design eliminates most common threading bugs
- **Fallback**: Single-threaded execution mode for debugging complex issues

### Performance Regression Risks

- **Risk**: Threading overhead negating performance benefits
- **Mitigation**: Comprehensive benchmarking and profiling during development
- **Fallback**: Disable threading for systems where overhead exceeds benefits

### WebAssembly Compatibility Risks

- **Risk**: Threading API differences causing platform-specific bugs
- **Mitigation**: Identical API with different implementations, extensive cross-platform testing
- **Fallback**: Single-threaded execution provides guaranteed compatibility

This threading system provides a robust foundation for parallel ECS execution while maintaining compatibility with your
existing architecture and WebAssembly requirements. The design emphasizes extensibility and can grow with the engine's
evolving needs.
