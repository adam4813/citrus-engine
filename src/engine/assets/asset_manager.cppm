module;

#include <cstdint>
#include <filesystem>
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

        // Asset-relative path overloads (prepend assets directory)
        static std::optional<std::string> LoadTextFile(const std::string &path);

        static std::optional<std::vector<uint8_t>> LoadBinaryFile(const std::string &path);

        static bool SaveTextFile(const std::string &path, const std::string &content);

        static bool SaveBinaryFile(const std::string &path, const std::vector<uint8_t> &data);

        // Absolute/explicit path overloads (no assets directory prepending)
        static std::optional<std::string> LoadTextFile(const std::filesystem::path &absolute_path);

        static std::optional<std::vector<uint8_t>> LoadBinaryFile(const std::filesystem::path &absolute_path);

        static bool SaveTextFile(const std::filesystem::path &absolute_path, const std::string &content);

        static bool SaveBinaryFile(const std::filesystem::path &absolute_path, const std::vector<uint8_t> &data);
    };
}
