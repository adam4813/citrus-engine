// Timing implementation stub
module;

#include <chrono>
#include <thread>

module engine.platform;

namespace engine::platform {

    // FrameRateController implementation
    FrameRateController::FrameRateController(double target_fps)
        : target_fps_(target_fps)
        , target_frame_time_(std::chrono::duration_cast<Duration>(std::chrono::duration<double>(1.0 / target_fps)))
        , frame_times_(60) // Store last 60 frame times for averaging
        , frame_index_(0) {
    }

    void FrameRateController::FrameStart() {
        frame_start_time_ = std::chrono::high_resolution_clock::now();
    }

    void FrameRateController::FrameEnd() {
        auto frame_end_time = std::chrono::high_resolution_clock::now();
        auto actual_frame_time = std::chrono::duration_cast<Duration>(frame_end_time - frame_start_time_);

        // Store frame time for averaging
        frame_times_[frame_index_] = actual_frame_time;
        frame_index_ = (frame_index_ + 1) % frame_times_.size();

        // Sleep if we finished early
        if (actual_frame_time < target_frame_time_) {
            auto sleep_time = target_frame_time_ - actual_frame_time;
            std::this_thread::sleep_for(sleep_time);
        }
    }

    double FrameRateController::GetCurrentFps() const {
        if (frame_times_.empty()) return 0.0;
        auto latest_frame_time = frame_times_[(frame_index_ + frame_times_.size() - 1) % frame_times_.size()];
        if (latest_frame_time.count() == 0) return 0.0;
        return 1.0 / std::chrono::duration<double>(latest_frame_time).count();
    }

    double FrameRateController::GetAverageFps() const {
        if (frame_times_.empty()) return 0.0;

        Duration total_time{0};
        size_t valid_frames = 0;

        for (const auto& frame_time : frame_times_) {
            if (frame_time.count() > 0) {
                total_time += frame_time;
                valid_frames++;
            }
        }

        if (valid_frames == 0) return 0.0;

        auto average_frame_time = total_time / valid_frames;
        return 1.0 / std::chrono::duration<double>(average_frame_time).count();
    }

    Duration FrameRateController::GetFrameTime() const {
        if (frame_times_.empty()) return Duration{0};
        return frame_times_[(frame_index_ + frame_times_.size() - 1) % frame_times_.size()];
    }

    void FrameRateController::SetTargetFps(double fps) {
        target_fps_ = fps;
        target_frame_time_ = std::chrono::duration_cast<Duration>(std::chrono::duration<double>(1.0 / fps));
    }

} // namespace engine::platform
