module;

#include <flecs.h>
#include <iostream>
#include <nlohmann/json.hpp>
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

void ShaderAssetInfo::SetupRefBinding(flecs::world& world) {
	SetupRefBindingImpl<ShaderAssetInfo, ShaderRef, rendering::Renderable>(
			world, "ShaderRef", "Rendering", "ShaderRefResolve", ShaderAssetInfo::TYPE_NAME,
			[](const auto& asset, auto& target) { target.shader = asset->id; },
			[](auto& target) { target.shader = rendering::INVALID_SHADER; });
}

} // namespace engine::assets
