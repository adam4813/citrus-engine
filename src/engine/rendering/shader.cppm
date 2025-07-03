module;

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

export module engine.rendering:shader;

import :types;
import engine.platform;

export namespace engine::rendering {
    enum class ShaderType {
        Vertex,
        Fragment
    };

    struct ShaderCreateInfo {
        std::string vertex_source;
        std::string fragment_source;
        std::vector<std::string> defines; // Preprocessor defines
    };

    class Shader {
    public:
        Shader();

        ~Shader();

        // Non-copyable but movable
        Shader(const Shader &) = delete;

        Shader &operator=(const Shader &) = delete;

        Shader(Shader &&) noexcept;

        Shader &operator=(Shader &&) noexcept;

        bool Compile(const ShaderCreateInfo &info) const;

        bool IsValid() const;

        // Uniform setters
        void SetUniform(const std::string &name, int value) const;

        void SetUniform(const std::string &name, float value) const;

        void SetUniform(const std::string &name, const Vec2 &value) const;

        void SetUniform(const std::string &name, const Vec3 &value) const;

        void SetUniform(const std::string &name, const Vec4 &value) const;

        void SetUniform(const std::string &name, const Mat3 &value) const;

        void SetUniform(const std::string &name, const Mat4 &value) const;

        void SetTexture(const std::string &name, TextureId texture, uint32_t slot = 0) const;

        void Use() const;

        // Shader introspection
        std::vector<std::string> GetUniformNames() const;

        std::vector<std::string> GetAttributeNames() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class ShaderManager {
    public:
        ShaderManager();

        ~ShaderManager();

        // Shader creation
        ShaderId CreateShader(const std::string &name, const ShaderCreateInfo &info) const;

        ShaderId LoadShaderFromString(const std::string &name, const std::string &vertex_source,
                                      const std::string &fragment_source) const;

        ShaderId LoadShader(const std::string &name, const platform::fs::Path &vertex_path,
                            const platform::fs::Path &fragment_path) const;

        // Shader access
        Shader &GetShader(ShaderId id);

        const Shader &GetShader(ShaderId id) const;

        ShaderId FindShader(const std::string &name) const;

        // Hot-reload support
        void ReloadShader(ShaderId id);

        void ReloadAllShaders();

        // Shader operations
        bool IsValid(ShaderId id) const;

        void DestroyShader(ShaderId id) const;

        void Clear() const;

        // Get default shaders
        ShaderId GetDefault2DShader() const;

        ShaderId GetDefault3DShader() const;

        ShaderId GetUnlitShader() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };
}
