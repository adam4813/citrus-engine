module;

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

export module engine.capture;

/**
 * @file capture.cppm
 * @brief Screenshot and GIF recording system for Citrus Engine
 * 
 * This module provides functionality for capturing screenshots and recording
 * GIF animations from the current framebuffer.
 * 
 * @section Usage Examples
 * 
 * **Screenshot Example:**
 * @code
 * auto& capture = engine::capture::GetCaptureManager();
 * 
 * // Simple screenshot (PNG, auto-named)
 * capture.Screenshot();
 * 
 * // Custom name and format
 * capture.Screenshot("my_screenshot", ImageFormat::JPEG);
 * 
 * // Full control
 * ScreenshotOptions opts;
 * opts.format = ImageFormat::PNG;
 * opts.quality = 95;
 * opts.include_alpha = true;
 * capture.Screenshot("high_quality", opts);
 * @endcode
 * 
 * **GIF Recording Example:**
 * @code
 * auto& capture = engine::capture::GetCaptureManager();
 * 
 * // Start recording at 30 FPS, half resolution
 * capture.GifStart(30, 0.5f);
 * 
 * // In your main loop, capture frames
 * while (recording) {
 *     // ... render scene ...
 *     capture.CaptureFrame();  // Call once per frame
 * }
 * 
 * // Stop and save
 * capture.GifEnd();
 * capture.GifSave("my_animation");
 * 
 * // Or cancel if needed
 * capture.GifCancel();
 * @endcode
 * 
 * **UI Integration Example:**
 * @code
 * // In your UI code (not Dear ImGui - use your engine's UI system)
 * auto& capture = engine::capture::GetCaptureManager();
 * 
 * if (ui.ButtonClicked("Screenshot")) {
 *     capture.Screenshot();
 * }
 * 
 * if (!capture.IsGifRecording()) {
 *     if (ui.ButtonClicked("Start Recording")) {
 *         capture.GifStart(30, 0.5f);
 *     }
 * } else {
 *     auto status = capture.GifGetStatus();
 *     ui.Label("Recording: " + std::to_string(status.frame_count) + " frames");
 *     
 *     if (ui.ButtonClicked("Stop & Save")) {
 *         capture.GifEnd();
 *         capture.GifSave();
 *     }
 *     if (ui.ButtonClicked("Cancel")) {
 *         capture.GifCancel();
 *     }
 * }
 * @endcode
 */

export namespace engine::capture {

/**
 * @brief Image format for screenshots
 */
enum class ImageFormat {
    PNG,
    JPEG,
    BMP
};

/**
 * @brief GIF recording options
 */
struct GifOptions {
    uint32_t fps = 30;                    // Frames per second (5-60)
    float scale = 1.0f;                   // Scale factor (e.g., 0.5 for half size)
    uint32_t palette_size = 256;          // Color palette size (16-256)
    bool dither = true;                   // Apply dithering
    int32_t loop_count = 0;               // 0 = infinite loop, -1 = no loop
};

/**
 * @brief Screenshot options
 */
struct ScreenshotOptions {
    ImageFormat format = ImageFormat::PNG;
    int quality = 90;                     // Quality for JPEG (1-100)
    bool include_alpha = true;            // Include alpha channel (PNG only)
};

/**
 * @brief GIF recording status
 */
struct GifStatus {
    bool is_recording = false;
    uint32_t frame_count = 0;
    size_t memory_used = 0;               // Bytes used for buffered frames
    float duration = 0.0f;                // Duration in seconds
};

/**
 * @brief Capture Manager - handles screenshot and GIF recording
 * 
 * This class provides the core functionality for capturing screenshots
 * and recording GIF animations from the current framebuffer.
 */
class CaptureManager {
public:
    CaptureManager();
    ~CaptureManager();

    // Disable copy
    CaptureManager(const CaptureManager&) = delete;
    CaptureManager& operator=(const CaptureManager&) = delete;

    // Allow move
    CaptureManager(CaptureManager&&) noexcept;
    CaptureManager& operator=(CaptureManager&&) noexcept;

    /**
     * @brief Set the output directory for captures
     * @param directory Path to output directory
     * @return true if directory is valid/created
     */
    bool SetOutputDirectory(const std::string& directory);

    /**
     * @brief Get the current output directory
     */
    [[nodiscard]] const std::string& GetOutputDirectory() const;

    // Screenshot API
    
    /**
     * @brief Capture a screenshot with default settings and auto-generated filename
     * @return true if screenshot was saved successfully
     */
    bool Screenshot();

    /**
     * @brief Capture a screenshot with custom filename
     * @param filename Output filename (without extension)
     * @return true if screenshot was saved successfully
     */
    bool Screenshot(const std::string& filename);

    /**
     * @brief Capture a screenshot with custom filename and format
     * @param filename Output filename (without extension)
     * @param format Image format (PNG, JPEG, BMP)
     * @return true if screenshot was saved successfully
     */
    bool Screenshot(const std::string& filename, ImageFormat format);

    /**
     * @brief Capture a screenshot with full options
     * @param filename Output filename (without extension)
     * @param options Screenshot options
     * @return true if screenshot was saved successfully
     */
    bool Screenshot(const std::string& filename, const ScreenshotOptions& options);

    // GIF Recording API

    /**
     * @brief Start GIF recording with default settings
     * @return true if recording started successfully
     */
    bool GifStart();

    /**
     * @brief Start GIF recording with custom FPS
     * @param fps Frames per second (5-60)
     * @return true if recording started successfully
     */
    bool GifStart(uint32_t fps);

    /**
     * @brief Start GIF recording with custom FPS and scale
     * @param fps Frames per second (5-60)
     * @param scale Scale factor (0.1-1.0)
     * @return true if recording started successfully
     */
    bool GifStart(uint32_t fps, float scale);

    /**
     * @brief Start GIF recording with full options
     * @param options GIF recording options
     * @return true if recording started successfully
     */
    bool GifStart(const GifOptions& options);

    /**
     * @brief Stop recording and close the buffer (frames ready for save)
     * @return true if recording was stopped successfully
     */
    bool GifEnd();

    /**
     * @brief Save the recorded GIF with auto-generated filename
     * @return true if GIF was saved successfully
     */
    bool GifSave();

    /**
     * @brief Save the recorded GIF with custom filename
     * @param filename Output filename (without extension)
     * @return true if GIF was saved successfully
     */
    bool GifSave(const std::string& filename);

    /**
     * @brief Cancel recording and discard all buffered frames
     */
    void GifCancel();

    /**
     * @brief Get the current recording status
     * @return GIF recording status
     */
    [[nodiscard]] GifStatus GifGetStatus() const;

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    [[nodiscard]] bool IsGifRecording() const;

    /**
     * @brief Capture current frame for GIF (called internally during recording)
     * Should be called once per frame during active recording
     * @return true if frame was captured successfully
     */
    bool CaptureFrame();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Get the global capture manager instance
 * @return Reference to the capture manager
 */
CaptureManager& GetCaptureManager();

} // namespace engine::capture
