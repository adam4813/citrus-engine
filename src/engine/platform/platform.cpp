// Platform module implementation
module;

#include <chrono>
#include <thread>

module engine.platform;

namespace engine::platform {

    // Platform info implementation
    static PlatformInfo g_platform_info;

    const PlatformInfo& GetPlatformInfo() {
        static bool initialized = false;
        if (!initialized) {
            g_platform_info.type = PlatformType::Windows;
            g_platform_info.version = "Windows 10+";
            g_platform_info.architecture = "x64";
            g_platform_info.total_memory_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB default
            g_platform_info.cpu_core_count = std::thread::hardware_concurrency();
            g_platform_info.has_sse4_1 = true;  // Assume modern CPU
            g_platform_info.has_avx2 = true;
            initialized = true;
        }
        return g_platform_info;
    }

    // Timer implementation
    Timer::Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}

    TimePoint Timer::Now() {
        return std::chrono::high_resolution_clock::now();
    }

    void Timer::Start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    Duration Timer::Elapsed() const {
        return std::chrono::duration_cast<Duration>(std::chrono::high_resolution_clock::now() - start_time_);
    }

    void Timer::Reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    double Timer::ElapsedSeconds() const {
        return std::chrono::duration<double>(Elapsed()).count();
    }

    double Timer::ElapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>(Elapsed()).count();
    }

    double Timer::ElapsedMicroseconds() const {
        return std::chrono::duration<double, std::micro>(Elapsed()).count();
    }

} // namespace engine::platform
