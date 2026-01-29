module;

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

export module engine.rendering:shader;

import :types;
import engine.platform;

export namespace engine::rendering {
enum class ShaderType { Vertex, Fragment };

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
	Shader(const Shader&) = delete;

	Shader& operator=(const Shader&) = delete;

	Shader(Shader&&) noexcept;

	Shader& operator=(Shader&&) noexcept;

	bool Compile(const ShaderCreateInfo& info) const;

	bool IsValid() const;

	// Uniform setters
	void SetUniform(const std::string& name, int value) const;

	void SetUniform(const std::string& name, float value) const;

	void SetUniform(const std::string& name, const Vec2& value) const;

	void SetUniform(const std::string& name, const Vec3& value) const;

	void SetUniform(const std::string& name, const Vec4& value) const;

	void SetUniform(const std::string& name, const Mat3& value) const;

	void SetUniform(const std::string& name, const Mat4& value) const;

	void SetUniformArray(const std::string& name, const int* values, int count) const;

	void SetTexture(const std::string& name, TextureId texture, uint32_t slot = 0) const;

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

	/// Create an uncompiled shader slot with the given name
	/// @return ShaderId that can be used immediately (shader is invalid until compiled)
	ShaderId CreateShader(const std::string& name) const;

	/// Compile an existing shader from file paths
	/// @return true if compilation succeeded
	bool
	CompileShader(ShaderId id, const platform::fs::Path& vertex_path, const platform::fs::Path& fragment_path) const;

	/// Compile an existing shader from source strings
	/// @return true if compilation succeeded
	bool CompileShader(ShaderId id, const std::string& vertex_source, const std::string& fragment_source) const;

	/// Create and compile a shader in one step
	ShaderId LoadShaderFromString(
			const std::string& name, const std::string& vertex_source, const std::string& fragment_source) const;

	/// Create and compile a shader in one step
	ShaderId LoadShader(
			const std::string& name,
			const platform::fs::Path& vertex_path,
			const platform::fs::Path& fragment_path) const;

	// Shader access
	Shader& GetShader(ShaderId id);

	const Shader& GetShader(ShaderId id) const;

	[[nodiscard]] ShaderId FindShader(const std::string& name) const;

	[[nodiscard]] std::string GetShaderName(ShaderId id) const;

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
} // namespace engine::rendering
