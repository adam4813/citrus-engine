module;

#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

module engine.asset_registry;

namespace engine::assets {

// === AnimationAssetInfo ===
void AnimationAssetInfo::DoInitialize() {
	// Stub: no initialization needed yet
}

bool AnimationAssetInfo::DoLoad() {
	// Stub: load animation clip data when animation system is available
	return true;
}

void AnimationAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void AnimationAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void AnimationAssetInfo::RegisterType() {
	AssetTypeRegistry::Instance()
			.RegisterType<AnimationAssetInfo>(AnimationAssetInfo::TYPE_NAME, AssetType::ANIMATION_CLIP)
			.DisplayName("Animation Clip")
			.Category("Animation")
			.Field("name", &AnimationAssetInfo::name, "Name")
			.Field("file_path", &AnimationAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

// === DataTableAssetInfo ===
void DataTableAssetInfo::DoInitialize() {
	// Stub: no data table system yet
}

bool DataTableAssetInfo::DoLoad() {
	// Stub: load JSON data when data table system is available
	return true;
}

void DataTableAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	schema_name = j.value("schema_name", "");
	AssetInfo::FromJson(j);
}

void DataTableAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	j["schema_name"] = schema_name;
	AssetInfo::ToJson(j);
}

void DataTableAssetInfo::RegisterType() {
	AssetTypeRegistry::Instance()
			.RegisterType<DataTableAssetInfo>(DataTableAssetInfo::TYPE_NAME, AssetType::DATA_TABLE)
			.DisplayName("Data Table")
			.Category("Data")
			.Field("name", &DataTableAssetInfo::name, "Name")
			.Field("file_path", &DataTableAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Field("schema_name", &DataTableAssetInfo::schema_name, "Schema Name")
			.Build();
}

// === PrefabAssetInfo ===
void PrefabAssetInfo::DoInitialize() {
	// Stub: prefab initialization handled by PrefabUtility
}

bool PrefabAssetInfo::DoLoad() {
	// Stub: prefab loading handled by PrefabUtility
	return true;
}

void PrefabAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void PrefabAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void PrefabAssetInfo::RegisterType() {
	AssetTypeRegistry::Instance()
			.RegisterType<PrefabAssetInfo>(PrefabAssetInfo::TYPE_NAME, AssetType::PREFAB)
			.DisplayName("Prefab")
			.Category("Scene")
			.Field("name", &PrefabAssetInfo::name, "Name")
			.Field("file_path", &PrefabAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

} // namespace engine::assets
