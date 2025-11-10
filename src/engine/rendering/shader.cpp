// Shader implementation stub
module;

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include <iostream>

module engine.rendering;

import engine.assets;
import engine.platform;
import glm;

namespace engine::rendering {
    // Shader implementation
    struct Shader::Impl {
        bool valid = false;
        std::string vertex_source;
        std::string fragment_source;
        std::unordered_map<std::string, int> uniform_locations;
        GLuint program;
    };

    Shader::Shader() : pimpl_(std::make_unique<Impl>()) {
    }

    Shader::~Shader() = default;

    Shader::Shader(Shader &&) noexcept = default;

    Shader &Shader::operator=(Shader &&) noexcept = default;

    bool Shader::Compile(const ShaderCreateInfo &info) const {
        pimpl_->vertex_source = info.vertex_source;
        pimpl_->fragment_source = info.fragment_source;

        // Compile vertex shader
        const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        const char *vertex_src = pimpl_->vertex_source.c_str();
        glShaderSource(vertex_shader, 1, &vertex_src, nullptr);
        glCompileShader(vertex_shader);
        GLint vertex_compiled = 0;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_compiled);
        if (!vertex_compiled) {
            char log[512];
            glGetShaderInfoLog(vertex_shader, 512, nullptr, log);
            std::cerr << "Vertex shader compilation failed: " << log << std::endl;
            glDeleteShader(vertex_shader);
            pimpl_->valid = false;
            return false;
        }

        // Compile fragment shader
        const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragment_src = pimpl_->fragment_source.c_str();
        glShaderSource(fragment_shader, 1, &fragment_src, nullptr);
        glCompileShader(fragment_shader);
        GLint fragment_compiled = 0;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_compiled);
        if (!fragment_compiled) {
            char log[512];
            glGetShaderInfoLog(fragment_shader, 512, nullptr, log);
            std::cerr << "Fragment shader compilation failed: " << log << std::endl;
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            pimpl_->valid = false;
            return false;
        }

        // Link program
        const GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[512];
            glGetProgramInfoLog(program, 512, nullptr, log);
            std::cerr << "Shader program linking failed: " << log << std::endl;
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            glDeleteProgram(program);
            pimpl_->valid = false;
            return false;
        }

        // Clean up shaders (no longer needed after linking)
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // Store program handle
        pimpl_->program = program;
        pimpl_->valid = true;
        return true;
    }

    bool Shader::IsValid() const {
        return pimpl_->valid;
    }

    void Shader::SetUniform(const std::string &name, const int value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glUniform1i(location, value);
    }

    void Shader::SetUniform(const std::string &name, float value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glUniform1f(location, value);
    }

    void Shader::SetUniform(const std::string &name, const Vec2 &value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glUniform2fv(location, 1, &value[0]);
    }

    void Shader::SetUniform(const std::string &name, const Vec3 &value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glUniform3fv(location, 1, &value[0]);
    }

    void Shader::SetUniform(const std::string &name, const Vec4 &value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glUniform4fv(location, 1, &value[0]);
    }

    void Shader::SetUniform(const std::string &name, const Mat3 &value) const {
        // TODO: Implement uniform setting
    }

    void Shader::SetUniform(const std::string &name, const Mat4 &value) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            pimpl_->uniform_locations[name] = glGetUniformLocation(pimpl_->program, name.c_str());
        }
        location = pimpl_->uniform_locations[name];

        if (location != -1) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void Shader::SetUniformArray(const std::string &name, const int* values, int count) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        } else {
            location = pimpl_->uniform_locations[name];
        }
        if (location != -1) {
            glUniform1iv(location, count, values);
        }
    }

    void Shader::SetTexture(const std::string &name, const TextureId texture, const uint32_t slot) const {
        GLint location = -1;
        if (!pimpl_->uniform_locations.contains(name)) {
            location = glGetUniformLocation(pimpl_->program, name.c_str());
            pimpl_->uniform_locations[name] = location;
        }
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture); // Assuming texture is a GLuint handle
        glUniform1i(location, slot);
    }

    void Shader::Use() const {
        if (pimpl_->valid) {
            glUseProgram(pimpl_->program);
        } else {
            std::cerr << "Shader program is not valid!" << std::endl;
        }
    }

    std::vector<std::string> Shader::GetUniformNames() const {
        return {}; // TODO: Return actual uniform names
    }

    std::vector<std::string> Shader::GetAttributeNames() const {
        return {}; // TODO: Return actual attribute names
    }

    // ShaderManager implementation
    struct ShaderManager::Impl {
        std::unordered_map<ShaderId, std::unique_ptr<Shader> > shaders;
        std::unordered_map<std::string, ShaderId> name_to_id;
        ShaderId next_id = 1;
    };

    ShaderManager::ShaderManager() : pimpl_(std::make_unique<Impl>()) {
    }

    ShaderManager::~ShaderManager() = default;

    ShaderId ShaderManager::CreateShader(const std::string &name, const ShaderCreateInfo &info) const {
        auto shader = std::make_unique<Shader>();
        if (!shader->Compile(info)) {
            return INVALID_SHADER;
        }

        const ShaderId id = pimpl_->next_id++;
        pimpl_->shaders[id] = std::move(shader);
        pimpl_->name_to_id[name] = id;
        return id;
    }

    ShaderId ShaderManager::LoadShaderFromString(const std::string &name, const std::string &vertex_source,
                                                 const std::string &fragment_source) const {
        ShaderCreateInfo info;
        info.vertex_source = vertex_source;
        info.fragment_source = fragment_source;
        return CreateShader(name, info);
    }

    ShaderId ShaderManager::LoadShader(const std::string &name, const platform::fs::Path &vertex_path,
                                       const platform::fs::Path &fragment_path) const {
        // Use asset manager to load shader source files
        auto &asset_manager = assets::AssetManager::Instance();
        const auto vertex_src_opt = asset_manager.LoadTextFile(vertex_path.string());
        const auto fragment_src_opt = asset_manager.LoadTextFile(fragment_path.string());
        if (!vertex_src_opt || !fragment_src_opt) {
            return INVALID_SHADER;
        }
        std::cout << "Loaded shader sources: " << vertex_path << ", " << fragment_path << std::endl;
        return LoadShaderFromString(name, *vertex_src_opt, *fragment_src_opt);
    }

    Shader &ShaderManager::GetShader(const ShaderId id) {
        if (const auto it = pimpl_->shaders.find(id); it != pimpl_->shaders.end()) {
            return *it->second;
        }
        static Shader invalid_shader;
        return invalid_shader;
    }

    const Shader &ShaderManager::GetShader(const ShaderId id) const {
        if (const auto it = pimpl_->shaders.find(id); it != pimpl_->shaders.end()) {
            return *it->second;
        }
        static Shader invalid_shader;
        return invalid_shader;
    }

    ShaderId ShaderManager::FindShader(const std::string &name) const {
        const auto it = pimpl_->name_to_id.find(name);
        return it != pimpl_->name_to_id.end() ? it->second : INVALID_SHADER;
    }

    void ShaderManager::ReloadShader(ShaderId id) {
        // TODO: Implement shader reloading
    }

    void ShaderManager::ReloadAllShaders() {
        // TODO: Implement all shader reloading
    }

    void ShaderManager::DestroyShader(const ShaderId id) const {
        pimpl_->shaders.erase(id);
    }

    bool ShaderManager::IsValid(const ShaderId id) const {
        return pimpl_->shaders.contains(id);
    }

    void ShaderManager::Clear() const {
        pimpl_->shaders.clear();
        pimpl_->name_to_id.clear();
    }

    ShaderId ShaderManager::GetDefault2DShader() const {
        return 1; // TODO: Return actual default 2D shader
    }

    ShaderId ShaderManager::GetDefault3DShader() const {
        return 2; // TODO: Return actual default 3D shader
    }

    ShaderId ShaderManager::GetUnlitShader() const {
        return 3; // TODO: Return actual unlit shader
    }
} // namespace engine::rendering
