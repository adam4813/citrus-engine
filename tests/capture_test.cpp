#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

import engine.capture;

class CaptureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test directory
        test_dir_ = "test_captures";
        std::filesystem::create_directories(test_dir_);
        
        // Get the capture manager and set test directory
        auto& manager = engine::capture::GetCaptureManager();
        manager.SetOutputDirectory(test_dir_);
    }

    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
};

TEST_F(CaptureTest, capture_manager_initializes) {
    auto& manager = engine::capture::GetCaptureManager();
    EXPECT_EQ(manager.GetOutputDirectory(), test_dir_);
}

TEST_F(CaptureTest, set_output_directory_creates_directory) {
    auto& manager = engine::capture::GetCaptureManager();
    
    const std::string new_dir = test_dir_ + "/subdirectory";
    EXPECT_TRUE(manager.SetOutputDirectory(new_dir));
    EXPECT_TRUE(std::filesystem::exists(new_dir));
    EXPECT_EQ(manager.GetOutputDirectory(), new_dir);
}

TEST_F(CaptureTest, gif_status_initial_state) {
    auto& manager = engine::capture::GetCaptureManager();
    
    auto status = manager.GifGetStatus();
    EXPECT_FALSE(status.is_recording);
    EXPECT_EQ(status.frame_count, 0u);
    EXPECT_EQ(status.memory_used, 0u);
    EXPECT_FLOAT_EQ(status.duration, 0.0f);
}

TEST_F(CaptureTest, gif_start_default_settings) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_FALSE(manager.IsGifRecording());
    EXPECT_TRUE(manager.GifStart());
    EXPECT_TRUE(manager.IsGifRecording());
    
    auto status = manager.GifGetStatus();
    EXPECT_TRUE(status.is_recording);
    EXPECT_EQ(status.frame_count, 0u);
}

TEST_F(CaptureTest, gif_start_with_custom_fps) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_TRUE(manager.GifStart(60));
    EXPECT_TRUE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_start_with_fps_and_scale) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_TRUE(manager.GifStart(30, 0.5f));
    EXPECT_TRUE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_start_rejects_invalid_fps) {
    auto& manager = engine::capture::GetCaptureManager();
    
    // FPS too low
    EXPECT_FALSE(manager.GifStart(4));
    EXPECT_FALSE(manager.IsGifRecording());
    
    // FPS too high
    EXPECT_FALSE(manager.GifStart(61));
    EXPECT_FALSE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_start_rejects_invalid_scale) {
    auto& manager = engine::capture::GetCaptureManager();
    
    // Scale too low
    EXPECT_FALSE(manager.GifStart(30, 0.0f));
    EXPECT_FALSE(manager.IsGifRecording());
    
    // Scale too high
    EXPECT_FALSE(manager.GifStart(30, 1.1f));
    EXPECT_FALSE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_start_rejects_while_already_recording) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_TRUE(manager.GifStart());
    EXPECT_TRUE(manager.IsGifRecording());
    
    // Try to start again while recording
    EXPECT_FALSE(manager.GifStart());
    EXPECT_TRUE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_end_stops_recording) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_TRUE(manager.GifStart());
    EXPECT_TRUE(manager.IsGifRecording());
    
    EXPECT_TRUE(manager.GifEnd());
    EXPECT_FALSE(manager.IsGifRecording());
}

TEST_F(CaptureTest, gif_end_without_start_returns_false) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_FALSE(manager.IsGifRecording());
    EXPECT_FALSE(manager.GifEnd());
}

TEST_F(CaptureTest, gif_cancel_clears_state) {
    auto& manager = engine::capture::GetCaptureManager();
    
    EXPECT_TRUE(manager.GifStart());
    EXPECT_TRUE(manager.IsGifRecording());
    
    manager.GifCancel();
    EXPECT_FALSE(manager.IsGifRecording());
    
    auto status = manager.GifGetStatus();
    EXPECT_FALSE(status.is_recording);
    EXPECT_EQ(status.frame_count, 0u);
    EXPECT_EQ(status.memory_used, 0u);
}

TEST_F(CaptureTest, image_format_enum_values) {
    using engine::capture::ImageFormat;
    
    // Just verify the enum values exist and are distinct
    EXPECT_NE(static_cast<int>(ImageFormat::PNG), static_cast<int>(ImageFormat::JPEG));
    EXPECT_NE(static_cast<int>(ImageFormat::PNG), static_cast<int>(ImageFormat::BMP));
    EXPECT_NE(static_cast<int>(ImageFormat::JPEG), static_cast<int>(ImageFormat::BMP));
}

TEST_F(CaptureTest, gif_options_default_values) {
    engine::capture::GifOptions options;
    
    EXPECT_EQ(options.fps, 30u);
    EXPECT_FLOAT_EQ(options.scale, 1.0f);
    EXPECT_EQ(options.palette_size, 256u);
    EXPECT_TRUE(options.dither);
    EXPECT_EQ(options.loop_count, 0);
}

TEST_F(CaptureTest, screenshot_options_default_values) {
    engine::capture::ScreenshotOptions options;
    
    EXPECT_EQ(options.format, engine::capture::ImageFormat::PNG);
    EXPECT_EQ(options.quality, 90);
    EXPECT_TRUE(options.include_alpha);
}
