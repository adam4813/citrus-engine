module;

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <optional>

export module engine.assets:asset_manager;

export namespace engine::assets {
    struct Image {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::string name;
        std::vector<uint8_t> pixel_data;
        bool IsValid() const { return width > 0 && height > 0 && !pixel_data.empty(); }
    };

    class AssetManager {
    public:
        static AssetManager &Instance();

        static std::shared_ptr<Image> LoadImage(const std::string &path);

        static std::optional<std::string> LoadTextFile(const std::string &path);

        /**
         * @brief Load a binary file from the assets directory
         * @param path Path relative to assets directory
         * @return Binary data as vector of bytes, or empty vector if loading fails
         * 
         * This method loads any binary file (fonts, audio, etc.) without decoding.
         * Suitable for font files (.ttf, .otf) and other binary assets.
         */
        static std::vector<uint8_t> LoadBinaryFile(const std::string &path);
    };
}
