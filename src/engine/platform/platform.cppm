module;

// Platform-specific includes
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <span>
#include <cstdint>
#include <fstream>

export module engine.platform;

export namespace engine::platform {
    // =============================================================================
    // Platform Detection and Information
    // =============================================================================

    enum class PlatformType {
        Windows,
        Linux,
        MacOS,
        WebAssembly
    };

    struct PlatformInfo {
        PlatformType type;
        std::string version;
        std::string architecture;
        size_t total_memory_bytes;
        uint32_t cpu_core_count;
        bool has_sse4_1;
        bool has_avx2;
    };

    // Get current platform information
    const PlatformInfo &GetPlatformInfo();

    // =============================================================================
    // High-Resolution Timing
    // =============================================================================

    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;

    class Timer {
    public:
        Timer();

        // Get current high-resolution time
        static TimePoint Now();

        // Start/restart the timer
        void Start();

        // Get elapsed time since start
        Duration Elapsed() const;

        // Reset timer to zero
        void Reset();

        // Get elapsed time in common units
        double ElapsedSeconds() const;

        double ElapsedMilliseconds() const;

        double ElapsedMicroseconds() const;

    private:
        TimePoint start_time_;
    };

    // Frame rate management
    class FrameRateController {
    public:
        explicit FrameRateController(double target_fps = 60.0);

        // Call at beginning of frame
        void FrameStart();

        // Call at end of frame - will sleep if needed to maintain target FPS
        void FrameEnd();

        // Get current FPS metrics
        double GetCurrentFps() const;

        double GetAverageFps() const;

        Duration GetFrameTime() const;

        void SetTargetFps(double fps);

    private:
        double target_fps_;
        Duration target_frame_time_;
        TimePoint frame_start_time_;
        // Moving average for FPS calculation
        std::vector<Duration> frame_times_;
        size_t frame_index_;
    };

    // =============================================================================
    // File System Abstraction
    // =============================================================================

    namespace fs {
        using Path = std::filesystem::path;

        enum class FileMode {
            Read,
            Write,
            Append,
            ReadWrite
        };

        enum class FileType {
            Binary,
            Text
        };

        class File {
        public:
            File() = default;

            ~File();

            // Non-copyable but movable
            File(const File &) = delete;

            File &operator=(const File &) = delete;

            File(File &&) noexcept;

            File &operator=(File &&) noexcept;

            // Open/close file
            bool Open(const Path &path, FileMode mode, FileType type = FileType::Binary);

            void Close();

            bool IsOpen() const;

            // Read operations
            size_t Read(void *buffer, size_t size);

            std::vector<uint8_t> ReadAll();

            std::string ReadText();

            // Write operations
            size_t Write(const void *data, size_t size);

            bool WriteText(const std::string &text);

            // File positioning
            bool Seek(size_t position);

            size_t Tell() const;

            size_t Size() const;

        private:
            //struct Impl;
            //std::unique_ptr<Impl> pimpl_;
            std::fstream file_stream_;
        };

        // Utility functions
        bool Exists(const Path &path);

        bool IsFile(const Path &path);

        bool IsDirectory(const Path &path);

        bool CreateDirectory(const Path &path);

        bool Remove(const Path &path);

        size_t FileSize(const Path &path);

        std::vector<Path> ListDirectory(const Path &path);

        // Get standard paths
        Path GetExecutablePath();

        Path GetWorkingDirectory();

        Path GetAssetsDirectory();

        // File watching (for hot-reload)
        using FileWatchCallback = std::function<void(const Path &)>;

        class FileWatcher {
        public:
            FileWatcher();

            ~FileWatcher();

            // Non-copyable but movable
            FileWatcher(const FileWatcher &) = delete;

            FileWatcher &operator=(const FileWatcher &) = delete;

            FileWatcher(FileWatcher &&) noexcept;

            FileWatcher &operator=(FileWatcher &&) noexcept;

            bool WatchFile(const Path &path, FileWatchCallback callback);

            bool WatchDirectory(const Path &path, FileWatchCallback callback);

            void StopWatching(const Path &path);

            void StopAll();

            // Poll for file changes (call regularly)
            void Poll();

        private:
            struct Impl;
            std::unique_ptr<Impl> pimpl_;
        };
    }

    // =============================================================================
    // Memory Management
    // =============================================================================

    namespace memory {
        // Memory alignment utilities
        constexpr size_t default_alignment = alignof(std::max_align_t);

        template<size_t Alignment = default_alignment>
        constexpr bool IsAligned(const void *ptr) {
            return reinterpret_cast<uintptr_t>(ptr) % Alignment == 0;
        }

        template<size_t Alignment = default_alignment>
        constexpr size_t AlignSize(size_t size) {
            return (size + Alignment - 1) & ~(Alignment - 1);
        }

        // Custom allocator interface
        class Allocator {
        public:
            virtual ~Allocator() = default;

            virtual void *Allocate(size_t size, size_t alignment = default_alignment) = 0;

            virtual void Deallocate(void *ptr) = 0;

            virtual size_t AllocatedSize() const = 0;

            virtual size_t PeakSize() const = 0;
        };

        // Linear allocator (fast, no individual deallocation)
        class LinearAllocator : public Allocator {
        public:
            explicit LinearAllocator(size_t capacity);

            ~LinearAllocator() override;

            void *Allocate(size_t size, size_t alignment = default_alignment) override;

            void Deallocate(void *ptr) override; // No-op for linear allocator

            void Reset(); // Reset to beginning
            size_t AllocatedSize() const override;

            size_t PeakSize() const override;

            size_t Remaining() const;

        private:
            uint8_t *buffer_;
            size_t capacity_;
            size_t offset_;
            size_t peak_offset_;
        };

        // Pool allocator (fixed-size blocks)
        class PoolAllocator : public Allocator {
        public:
            PoolAllocator(size_t block_size, size_t block_count);

            ~PoolAllocator() override;

            void *Allocate(size_t size, size_t alignment = default_alignment) override;

            void Deallocate(void *ptr) override;

            size_t AllocatedSize() const override;

            size_t PeakSize() const override;

            size_t BlockSize() const { return block_size_; }

            size_t AvailableBlocks() const;

        private:
            struct FreeBlock {
                FreeBlock *next;
            };

            uint8_t *buffer_;
            FreeBlock *free_list_;
            size_t block_size_;
            size_t block_count_;
            size_t allocated_blocks_;
            size_t peak_allocated_;
        };

        // Get default allocators
        Allocator &GetDefaultAllocator();

        LinearAllocator &GetFrameAllocator(); // Reset every frame
    }

    // =============================================================================
    // Threading Primitives
    // =============================================================================

    namespace threading {
        // Thread utilities
        uint32_t HardwareConcurrency();

        void SetCurrentThreadName(const std::string &name);

        void YieldCurrentThread();

        void SleepFor(Duration duration);

        // Lightweight mutex for WebAssembly compatibility
        class Mutex {
        public:
            Mutex();

            ~Mutex();

            // Non-copyable, non-movable
            Mutex(const Mutex &) = delete;

            Mutex &operator=(const Mutex &) = delete;

            void Lock();

            bool TryLock();

            void Unlock();

        private:
            struct Impl;
            std::unique_ptr<Impl> pimpl_;
        };

        // RAII lock guard
        class LockGuard {
        public:
            explicit LockGuard(Mutex &mutex);

            ~LockGuard();

            // Non-copyable, non-movable
            LockGuard(const LockGuard &) = delete;

            LockGuard &operator=(const LockGuard &) = delete;

        private:
            Mutex &mutex_;
        };

        // Atomic primitives (WebAssembly compatible subset)
        template<typename T>
        class Atomic {
        public:
            Atomic() = default;

            explicit Atomic(T value);

            T Load() const;

            void Store(T value);

            T Exchange(T value);

            bool CompareExchange(T &expected, T desired);

            // For integral types
            T FetchAdd(T value) requires std::is_integral_v<T>;

            T FetchSub(T value) requires std::is_integral_v<T>;

        private:
            mutable std::atomic<T> value_;
        };

        // Common atomic types
        using AtomicInt32 = Atomic<int32_t>;
        using AtomicUInt32 = Atomic<uint32_t>;
        using AtomicInt64 = Atomic<int64_t>;
        using AtomicUInt64 = Atomic<uint64_t>;
        using AtomicBool = Atomic<bool>;
    }

    // =============================================================================
    // Utility Functions
    // =============================================================================

    // Debug and logging utilities
    void DebugBreak();

    bool IsDebuggerAttached();

    // Random number generation
    namespace random {
        void Seed(uint64_t seed);

        uint32_t NextUInt32();

        uint64_t NextUInt64();

        float NextFloat(); // [0.0f, 1.0f)
        double NextDouble(); // [0.0, 1.0)

        // Range functions
        int32_t RangeInt(int32_t min, int32_t max);

        float RangeFloat(float min, float max);
    }

    // Hash utilities
    namespace hash {
        uint32_t Fnv1a32(const void *data, size_t size);

        uint64_t Fnv1a64(const void *data, size_t size);

        template<typename T>
        uint32_t Hash32(const T &value) {
            return Fnv1a32(&value, sizeof(T));
        }

        template<typename T>
        uint64_t Hash64(const T &value) {
            return Fnv1a64(&value, sizeof(T));
        }
    }
} // namespace engine::platform
