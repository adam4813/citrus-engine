module;

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <optional>
#include <stb_image.h>

module engine.assets;

import engine.platform;

namespace engine::assets {
    AssetManager &AssetManager::Instance() {
        static AssetManager instance;
        return instance;
    }

    std::shared_ptr<Image> AssetManager::LoadImage(const std::string &path) {
        using namespace engine::platform;
        // Build full path to asset
        const fs::Path asset_path = fs::GetAssetsDirectory() / path;
        std::cout << "Loading image from: " << asset_path.string() << std::endl;
        fs::File file;
        if (!file.Open(asset_path, fs::FileMode::Read)) {
            // Could not open file
            return nullptr;
        }
        const std::vector<uint8_t> file_data = file.ReadAll();
        if (file_data.empty()) {
            // File is empty or failed to read
            return nullptr;
        }
        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc *pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &width, &height,
                                                &channels, 4);
        if (!pixels) {
            // stb_image failed to decode
            return nullptr;
        }
        auto image = std::make_shared<Image>();
        image->width = width;
        image->height = height;
        image->channels = 4; // Force RGBA
        image->pixel_data.assign(pixels, pixels + (width * height * 4));
        stbi_image_free(pixels);
        return image;
    }

    std::optional<std::string> AssetManager::LoadTextFile(const std::string &path) {
        using namespace engine::platform;
        const fs::Path asset_path = fs::GetAssetsDirectory() / path;
        fs::File file;
        if (!file.Open(asset_path, fs::FileMode::Read, fs::FileType::Text)) {
            return std::nullopt;
        }
        std::string text = file.ReadText();
        if (text.empty()) {
            return std::nullopt;
        }
        return text;
    }
} // namespace engine::assets
