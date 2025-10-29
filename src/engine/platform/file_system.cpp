// File system implementation stub
module;

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

module engine.platform;

namespace engine::platform::fs {
    // File implementation stub
    File::~File() {
        Close();
    }

    File::File(File &&) noexcept = default;

    File &File::operator=(File &&) noexcept = default;

    bool File::Open(const Path &path, const FileMode mode, FileType type) {
        const std::string mode_str = (mode == FileMode::Read)
                                         ? "r"
                                         : (mode == FileMode::Write)
                                               ? "w"
                                               : (mode == FileMode::Append)
                                                     ? "a"
                                                     : "r+";
        const std::string path_str = path.string();
        if (type == FileType::Binary) {
            file_stream_.open(path_str, std::ios::binary | (mode == FileMode::Read ? std::ios::in : std::ios::out));
        } else if (type == FileType::Text) {
            file_stream_.open(path_str, mode == FileMode::Read ? std::ios::in : std::ios::out);
        } else {
            // Unsupported file type
            return false;
        }
        return file_stream_.is_open();
    }

    void File::Close() {
        file_stream_.close();
    }

    bool File::IsOpen() const {
        return file_stream_.is_open();
    }

    size_t File::Read(void *buffer, size_t size) {
        return 0;
    }

    std::vector<uint8_t> File::ReadAll() {
        return file_stream_.eof()
                   ? std::vector<uint8_t>()
                   : std::vector<uint8_t>(std::istreambuf_iterator(file_stream_), {});
    }

    std::string File::ReadText() {
        return file_stream_.eof()
                   ? std::string()
                   : std::string(std::istreambuf_iterator(file_stream_), {});
    }

    size_t File::Write(const void *data, size_t size) {
        return 0;
    }

    bool File::WriteText(const std::string &text) {
        return false;
    }

    bool File::Seek(size_t position) {
        return false;
    }

    size_t File::Tell() const {
        return 0;
    }

    size_t File::Size() const {
        return 0;
    }

    Path GetAssetsDirectory() {
        return Path("assets");
    }
} // namespace engine::platform::fs
