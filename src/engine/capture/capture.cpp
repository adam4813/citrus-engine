// Capture Manager implementation
module;

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#include <GLFW/glfw3.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define MSF_GIF_IMPL
#include "msf_gif.h"

module engine.capture;

namespace engine::capture {

namespace {
    /**
     * @brief Generate a timestamp-based filename
     */
    std::string GenerateTimestampFilename(const std::string& prefix, const std::string& extension) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_buf);
#endif
        oss << prefix
            << std::put_time(&tm_buf, "%Y%m%d_%H%M%S")
            << "_" << std::setfill('0') << std::setw(3) << ms.count()
            << extension;
        return oss.str();
    }

    /**
     * @brief Read pixels from the current OpenGL framebuffer
     */
    bool ReadFramebuffer(uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels) {
        // Get current viewport dimensions
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        width = viewport[2];
        height = viewport[3];

        // Allocate buffer for pixel data (RGBA)
        pixels.resize(width * height * 4);

        // Read pixels from framebuffer
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            return false;
        }

        return true;
    }

    /**
     * @brief Flip image vertically (OpenGL reads bottom-up)
     */
    void FlipVertical(std::vector<uint8_t>& pixels, uint32_t width, uint32_t height) {
        const uint32_t row_size = width * 4;
        std::vector<uint8_t> temp_row(row_size);
        
        for (uint32_t y = 0; y < height / 2; ++y) {
            uint8_t* top = pixels.data() + y * row_size;
            uint8_t* bottom = pixels.data() + (height - 1 - y) * row_size;
            
            memcpy(temp_row.data(), top, row_size);
            memcpy(top, bottom, row_size);
            memcpy(bottom, temp_row.data(), row_size);
        }
    }

    /**
     * @brief Scale image down
     */
    void ScaleImage(const std::vector<uint8_t>& src, uint32_t src_width, uint32_t src_height,
                   std::vector<uint8_t>& dst, uint32_t dst_width, uint32_t dst_height) {
        dst.resize(dst_width * dst_height * 4);
        
        const float x_ratio = static_cast<float>(src_width) / dst_width;
        const float y_ratio = static_cast<float>(src_height) / dst_height;
        
        for (uint32_t y = 0; y < dst_height; ++y) {
            for (uint32_t x = 0; x < dst_width; ++x) {
                const uint32_t src_x = static_cast<uint32_t>(x * x_ratio);
                const uint32_t src_y = static_cast<uint32_t>(y * y_ratio);
                
                const uint32_t src_idx = (src_y * src_width + src_x) * 4;
                const uint32_t dst_idx = (y * dst_width + x) * 4;
                
                dst[dst_idx + 0] = src[src_idx + 0];
                dst[dst_idx + 1] = src[src_idx + 1];
                dst[dst_idx + 2] = src[src_idx + 2];
                dst[dst_idx + 3] = src[src_idx + 3];
            }
        }
    }
}

/**
 * @brief Internal implementation structure
 */
struct CaptureManager::Impl {
    std::string output_directory = "screenshots";
    
    // GIF recording state
    bool is_recording = false;
    GifOptions gif_options;
    std::vector<std::vector<uint8_t>> gif_frames;
    std::vector<float> gif_frame_times;
    float gif_accumulator = 0.0f;
    float gif_frame_duration = 0.0f;
    uint32_t gif_width = 0;
    uint32_t gif_height = 0;

    Impl() {
        // Create default output directory
        std::filesystem::create_directories(output_directory);
    }
};

CaptureManager::CaptureManager() : pimpl_(std::make_unique<Impl>()) {}

CaptureManager::~CaptureManager() = default;

CaptureManager::CaptureManager(CaptureManager&&) noexcept = default;

CaptureManager& CaptureManager::operator=(CaptureManager&&) noexcept = default;

bool CaptureManager::SetOutputDirectory(const std::string& directory) {
    try {
        std::filesystem::create_directories(directory);
        pimpl_->output_directory = directory;
        return true;
    } catch (...) {
        return false;
    }
}

const std::string& CaptureManager::GetOutputDirectory() const {
    return pimpl_->output_directory;
}

// Screenshot implementation

bool CaptureManager::Screenshot() {
    const std::string filename = GenerateTimestampFilename("screenshot_", "");
    return Screenshot(filename, ImageFormat::PNG);
}

bool CaptureManager::Screenshot(const std::string& filename) {
    return Screenshot(filename, ImageFormat::PNG);
}

bool CaptureManager::Screenshot(const std::string& filename, ImageFormat format) {
    ScreenshotOptions options;
    options.format = format;
    return Screenshot(filename, options);
}

bool CaptureManager::Screenshot(const std::string& filename, const ScreenshotOptions& options) {
    // Read framebuffer
    uint32_t width, height;
    std::vector<uint8_t> pixels;
    if (!ReadFramebuffer(width, height, pixels)) {
        return false;
    }

    // Flip image vertically (OpenGL coordinates)
    FlipVertical(pixels, width, height);

    // Build output path
    std::string extension;
    switch (options.format) {
        case ImageFormat::PNG: extension = ".png"; break;
        case ImageFormat::JPEG: extension = ".jpg"; break;
        case ImageFormat::BMP: extension = ".bmp"; break;
    }

    const std::string output_path = pimpl_->output_directory + "/" + filename + extension;

    // Write image file
    int result = 0;
    const int stride = width * 4;
    const int components = options.include_alpha ? 4 : 3;

    switch (options.format) {
        case ImageFormat::PNG:
            stbi_write_png_compression_level = 8;
            result = stbi_write_png(output_path.c_str(), width, height, components, pixels.data(), stride);
            break;
        case ImageFormat::JPEG:
            result = stbi_write_jpg(output_path.c_str(), width, height, components, pixels.data(), options.quality);
            break;
        case ImageFormat::BMP:
            result = stbi_write_bmp(output_path.c_str(), width, height, components, pixels.data());
            break;
    }

    return result != 0;
}

// GIF recording implementation

bool CaptureManager::GifStart() {
    return GifStart(30);
}

bool CaptureManager::GifStart(uint32_t fps) {
    return GifStart(fps, 1.0f);
}

bool CaptureManager::GifStart(uint32_t fps, float scale) {
    GifOptions options;
    options.fps = fps;
    options.scale = scale;
    return GifStart(options);
}

bool CaptureManager::GifStart(const GifOptions& options) {
    if (pimpl_->is_recording) {
        return false; // Already recording
    }

    // Validate options
    if (options.fps < 5 || options.fps > 60) {
        return false;
    }
    if (options.scale <= 0.0f || options.scale > 1.0f) {
        return false;
    }
    if (options.palette_size < 16 || options.palette_size > 256) {
        return false;
    }

    pimpl_->gif_options = options;
    pimpl_->gif_frames.clear();
    pimpl_->gif_frame_times.clear();
    pimpl_->gif_accumulator = 0.0f;
    pimpl_->gif_frame_duration = 1.0f / options.fps;
    pimpl_->is_recording = true;

    return true;
}

bool CaptureManager::GifEnd() {
    if (!pimpl_->is_recording) {
        return false;
    }

    pimpl_->is_recording = false;
    return true;
}

bool CaptureManager::CaptureFrame() {
    if (!pimpl_->is_recording) {
        return false;
    }

    // Read framebuffer
    uint32_t width, height;
    std::vector<uint8_t> pixels;
    if (!ReadFramebuffer(width, height, pixels)) {
        return false;
    }

    // Flip image vertically
    FlipVertical(pixels, width, height);

    // Store original dimensions on first frame
    if (pimpl_->gif_frames.empty()) {
        pimpl_->gif_width = width;
        pimpl_->gif_height = height;
    }

    // Scale if needed
    if (pimpl_->gif_options.scale < 1.0f) {
        const uint32_t scaled_width = static_cast<uint32_t>(width * pimpl_->gif_options.scale);
        const uint32_t scaled_height = static_cast<uint32_t>(height * pimpl_->gif_options.scale);
        std::vector<uint8_t> scaled_pixels;
        ScaleImage(pixels, width, height, scaled_pixels, scaled_width, scaled_height);
        pimpl_->gif_frames.push_back(std::move(scaled_pixels));
        pimpl_->gif_width = scaled_width;
        pimpl_->gif_height = scaled_height;
    } else {
        pimpl_->gif_frames.push_back(std::move(pixels));
    }

    return true;
}

bool CaptureManager::GifSave() {
    const std::string filename = GenerateTimestampFilename("recording_", ".gif");
    return GifSave(filename);
}

bool CaptureManager::GifSave(const std::string& filename) {
    if (pimpl_->gif_frames.empty()) {
        return false;
    }

    // Build output path
    std::string output_path = pimpl_->output_directory + "/" + filename;
    if (!output_path.ends_with(".gif")) {
        output_path += ".gif";
    }

    // Initialize GIF state
    MsfGifState gif_state = {};
    msf_gif_begin(&gif_state, pimpl_->gif_width, pimpl_->gif_height);

    // Calculate centiseconds per frame (GIF timing unit)
    const int centiseconds = static_cast<int>(100.0f / pimpl_->gif_options.fps);
    const int quality = 10; // Balance between quality and speed

    // Add frames
    for (const auto& frame : pimpl_->gif_frames) {
        const int pitch = pimpl_->gif_width * 4;
        msf_gif_frame(&gif_state, 
                     const_cast<uint8_t*>(frame.data()), 
                     centiseconds, 
                     quality, 
                     pitch);
    }

    // Finalize GIF
    MsfGifResult result = msf_gif_end(&gif_state);
    
    if (!result.data) {
        return false;
    }

    // Write to file
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        msf_gif_free(result);
        return false;
    }

    file.write(static_cast<const char*>(result.data), result.dataSize);
    file.close();

    msf_gif_free(result);

    // Clear frames after successful save
    pimpl_->gif_frames.clear();
    pimpl_->gif_frame_times.clear();

    return true;
}

void CaptureManager::GifCancel() {
    pimpl_->is_recording = false;
    pimpl_->gif_frames.clear();
    pimpl_->gif_frame_times.clear();
    pimpl_->gif_accumulator = 0.0f;
}

GifStatus CaptureManager::GifGetStatus() const {
    GifStatus status;
    status.is_recording = pimpl_->is_recording;
    status.frame_count = static_cast<uint32_t>(pimpl_->gif_frames.size());
    
    // Calculate memory usage
    size_t memory = 0;
    for (const auto& frame : pimpl_->gif_frames) {
        memory += frame.size();
    }
    status.memory_used = memory;
    
    // Calculate duration
    if (status.frame_count > 0) {
        status.duration = status.frame_count / static_cast<float>(pimpl_->gif_options.fps);
    }
    
    return status;
}

bool CaptureManager::IsGifRecording() const {
    return pimpl_->is_recording;
}

// Global instance
static std::unique_ptr<CaptureManager> g_capture_manager;

CaptureManager& GetCaptureManager() {
    if (!g_capture_manager) {
        g_capture_manager = std::make_unique<CaptureManager>();
    }
    return *g_capture_manager;
}

} // namespace engine::capture
