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

void MeshAssetInfo::DoInitialize() {
	const auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();
	id = mesh_mgr.CreateNamedMesh(name);
	std::cout << "MeshAssetInfo: Reserved mesh slot '" << name << "' (id=" << id << ")" << '\n';
}

bool MeshAssetInfo::DoLoad() {
	auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();

	bool success = false;
	if (mesh_type == mesh_types::QUAD) {
		success = mesh_mgr.GenerateQuad(id, params[0], params[1]);
	}
	else if (mesh_type == mesh_types::CUBE) {
		success = mesh_mgr.GenerateCube(id, params[0], params[1], params[2]);
	}
	else if (mesh_type == mesh_types::SPHERE) {
		success = mesh_mgr.GenerateSphere(id, params[0], static_cast<uint32_t>(params[1]));
	}
	else if (mesh_type == mesh_types::CAPSULE) {
		std::cerr << "MeshAssetInfo: Capsule mesh not yet implemented" << '\n';
		return false;
	}
	else if (mesh_type == mesh_types::FILE) {
		std::cerr << "MeshAssetInfo: File mesh loading not yet implemented: " << file_path << '\n';
		return false;
	}
	else {
		std::cerr << "MeshAssetInfo: Unknown mesh type: " << mesh_type << '\n';
		return false;
	}

	if (!success) {
		std::cerr << "MeshAssetInfo: Failed to generate mesh geometry for '" << name << "'" << '\n';
		return false;
	}

	std::cout << "MeshAssetInfo: Generated mesh '" << name << "' (type=" << mesh_type << ", id=" << id << ")" << '\n';
	return true;
}

void MeshAssetInfo::DoUnload() {
	const auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();
	mesh_mgr.DestroyMesh(id);
	std::cout << "MeshAssetInfo: Unloaded mesh '" << name << "' (id=" << id << ")" << '\n';
}

void MeshAssetInfo::FromJson(const nlohmann::json& j) {
	mesh_type = j.value("mesh_type", mesh_types::QUAD);
	if (j.contains("params") && j["params"].is_array()) {
		const auto& arr = j["params"];
		if (!arr.empty()) {
			params[0] = arr[0].get<float>();
		}
		if (arr.size() >= 2) {
			params[1] = arr[1].get<float>();
		}
		if (arr.size() >= 3) {
			params[2] = arr[2].get<float>();
		}
	}
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void MeshAssetInfo::ToJson(nlohmann::json& j) {
	j["mesh_type"] = mesh_type;
	j["params"] = {params[0], params[1], params[2]};
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void MeshAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<MeshAssetInfo>(MeshAssetInfo::TYPE_NAME, AssetType::MESH)
			.DisplayName("Mesh")
			.Category("Rendering")
			.Field("name", &MeshAssetInfo::name, "Name")
			.Field("mesh_type", &MeshAssetInfo::mesh_type, "Mesh Type", ecs::FieldType::Selection)
			.Options({mesh_types::QUAD, mesh_types::CUBE, mesh_types::SPHERE, mesh_types::CAPSULE, mesh_types::FILE})
			.Field("file_path", &MeshAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

void MeshAssetInfo::SetupRefBinding(flecs::world& world) {
	auto& registry = ecs::ComponentRegistry::Instance();
	registry.Register<MeshRef>("MeshRef", world)
			.Category("Rendering")
			.Field("name", &MeshRef::name)
			.AssetRef(MeshAssetInfo::TYPE_NAME)
			.Build();

	world.component<rendering::Renderable>().add(flecs::With, world.component<MeshRef>());

	world.observer<MeshRef, rendering::Renderable>("MeshRefResolve")
			.event(flecs::OnSet)
			.each([](flecs::entity, const MeshRef& ref, rendering::Renderable& target) {
				if (ref.name.empty()) {
					target.mesh = rendering::INVALID_MESH;
					return;
				}
				if (auto asset = AssetCache::Instance().FindTyped<MeshAssetInfo>(ref.name)) {
					asset->Load();
					target.mesh = asset->id;
				}
			});
}

} // namespace engine::assets
