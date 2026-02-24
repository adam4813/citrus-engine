module;

#include <flecs.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

module engine.asset_registry;

import engine.rendering;
import engine.platform;
import engine.ecs.component_registry;

namespace engine::assets {

void ShaderAssetInfo::DoInitialize() {
	const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
	id = shader_mgr.CreateShader(name);
	std::cout << "ShaderAssetInfo: Created shader slot '" << name << "' (id=" << id << ")" << '\n';
}

bool ShaderAssetInfo::DoLoad() {
	if (id == rendering::INVALID_SHADER) {
		std::cerr << "ShaderAssetInfo: Cannot load - shader not initialized" << '\n';
		return false;
	}

	if (const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
		!shader_mgr.CompileShader(id, platform::fs::Path(vertex_path), platform::fs::Path(fragment_path))) {
		std::cerr << "ShaderAssetInfo: Failed to compile shader '" << name << "'" << '\n';
		return false;
	}
	std::cout << "ShaderAssetInfo: Compiled shader '" << name << "' (id=" << id << ")" << '\n';
	return true;
}

void ShaderAssetInfo::DoUnload() {
	const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
	shader_mgr.DestroyShader(id);
	std::cout << "ShaderAssetInfo: Unloaded shader '" << name << "' (id=" << id << ")" << '\n';
}

void ShaderAssetInfo::FromJson(const nlohmann::json& j) {
	vertex_path = j.value("vertex_path", "");
	fragment_path = j.value("fragment_path", "");
	AssetInfo::FromJson(j);
}

void ShaderAssetInfo::ToJson(nlohmann::json& j) {
	j["vertex_path"] = vertex_path;
	j["fragment_path"] = fragment_path;
	AssetInfo::ToJson(j);
}

void ShaderAssetInfo::RegisterType() {
	AssetTypeRegistry::Instance()
			.RegisterType<ShaderAssetInfo>(ShaderAssetInfo::TYPE_NAME, AssetType::SHADER)
			.DisplayName("Shader")
			.Category("Rendering")
			.Field("name", &ShaderAssetInfo::name, "Name")
			.Field("vertex_path", &ShaderAssetInfo::vertex_path, "Vertex Shader", ecs::FieldType::FilePath)
			.Field("fragment_path", &ShaderAssetInfo::fragment_path, "Fragment Shader", ecs::FieldType::FilePath)
			.Build();
}

void ShaderAssetInfo::RegisterBuiltins() {
	auto& cache = AssetCache::Instance();
	const auto default_2d_shader = cache.Create<ShaderAssetInfo>(AssetType::SHADER, "__default_2d");
	default_2d_shader->vertex_path = "assets/shaders/basic.vert";
	default_2d_shader->fragment_path = "assets/shaders/basic.frag";

	const auto default_3d_shader = cache.Create<ShaderAssetInfo>(AssetType::SHADER, "__default_3d_lit");
	default_3d_shader->vertex_path = "assets/shaders/lit_3d.vert";
	default_3d_shader->fragment_path = "assets/shaders/lit_3d.frag";

	const auto unlit_shader = cache.Create<ShaderAssetInfo>(AssetType::SHADER, "__unlit");
	unlit_shader->vertex_path = "assets/shaders/unlit.vert";
	unlit_shader->fragment_path = "assets/shaders/unlit.frag";
}

void ShaderAssetInfo::SetupRefBinding(flecs::world& world) {
	SetupRefBindingImpl<ShaderAssetInfo, ShaderRef, rendering::Renderable>(
			world,
			"ShaderRef",
			"Rendering",
			"ShaderRefResolve",
			ShaderAssetInfo::TYPE_NAME,
			[](const auto& asset, auto& target) { target.shader = asset->id; },
			[](auto& target) { target.shader = rendering::INVALID_SHADER; });
}

} // namespace engine::assets
