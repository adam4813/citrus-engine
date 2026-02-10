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
        if (!file_stream_.is_open() || !buffer || size == 0) {
            return 0;
        }
        file_stream_.read(static_cast<char *>(buffer), static_cast<std::streamsize>(size));
        return static_cast<size_t>(file_stream_.gcount());
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
        if (!file_stream_.is_open() || !data || size == 0) {
            return 0;
        }
        file_stream_.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
        return file_stream_.good() ? size : 0;
    }

    bool File::WriteText(const std::string &text) {
        if (!file_stream_.is_open()) {
            return false;
        }
        file_stream_ << text;
        return file_stream_.good();
    }

    bool File::Seek(size_t position) {
        if (!file_stream_.is_open()) {
            return false;
        }
        file_stream_.seekg(static_cast<std::streamoff>(position), std::ios::beg);
        file_stream_.seekp(static_cast<std::streamoff>(position), std::ios::beg);
        return file_stream_.good();
    }

    size_t File::Tell() const {
        if (!file_stream_.is_open()) {
            return 0;
        }
        return static_cast<size_t>(const_cast<std::fstream &>(file_stream_).tellg());
    }

    size_t File::Size() const {
        if (!file_stream_.is_open()) {
            return 0;
        }
        auto &stream = const_cast<std::fstream &>(file_stream_);
        const auto current = stream.tellg();
        stream.seekg(0, std::ios::end);
        const auto size = static_cast<size_t>(stream.tellg());
        stream.seekg(current);
        return size;
    }

    Path GetAssetsDirectory() {
        return Path("assets");
    }
} // namespace engine::platform::fs
