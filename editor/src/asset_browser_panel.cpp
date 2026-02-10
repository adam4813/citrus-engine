#include "asset_browser_panel.h"

#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>

namespace editor {

void AssetBrowserPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void AssetBrowserPanel::Render(engine::scene::Scene* scene, const AssetSelection& selected_asset) {
	if (!is_visible_)
		return;

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
										 | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Assets", &is_visible_);

	if (!scene) {
		ImGui::TextDisabled("No scene loaded");
		ImGui::End();
		return;
	}

	// Iterate over all registered asset types and render their sections
	for (const auto& registry = engine::scene::AssetRegistry::Instance();
		 const auto& type_info : registry.GetAssetTypes()) {
		RenderAssetCategory(scene, type_info, selected_asset);
	}

	// Render prefab files section
	RenderPrefabSection();

	ImGui::End();
}

void AssetBrowserPanel::RenderAssetCategory(
		engine::scene::Scene* scene,
		const engine::scene::AssetTypeInfo& type_info,
		const AssetSelection& selected_asset) const {

	auto& assets = scene->GetAssets();

	// Count assets of this type
	size_t count = 0;
	for (const auto& asset : assets.GetAll()) {
		if (asset && asset->type == type_info.asset_type) {
			count++;
		}
	}

	// Build header label with count
	const std::string header_label = type_info.display_name + " (" + std::to_string(count) + ")";

	// Header with add button - use AllowItemOverlap to prevent button from triggering collapse
	const bool is_open = ImGui::CollapsingHeader(
			header_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

	// Add button (same line as header)
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::PushID(type_info.type_name.c_str());
	if (ImGui::SmallButton("+")) {
		ImGui::OpenPopup("AddAssetPopup");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Add new %s", type_info.display_name.c_str());
	}

	// Add asset popup (inside same PushID scope)
	if (ImGui::BeginPopup("AddAssetPopup")) {
		static char new_asset_name[64] = "";
		if (new_asset_name[0] == '\0') {
			std::strncpy(new_asset_name, ("New" + type_info.display_name).c_str(), sizeof(new_asset_name) - 1);
		}

		ImGui::Text("Create New %s", type_info.display_name.c_str());
		ImGui::Separator();
		ImGui::InputText("Name", new_asset_name, sizeof(new_asset_name));

		if (ImGui::Button("Create", ImVec2(100, 0)) && type_info.create_default_factory) {
			if (const auto new_asset = type_info.create_default_factory()) {
				new_asset->name = new_asset_name;
				assets.Add(new_asset);

				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
				if (callbacks_.on_asset_selected) {
					callbacks_.on_asset_selected(type_info.asset_type, new_asset_name);
				}
			}
			new_asset_name[0] = '\0'; // Reset for next use
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 0))) {
			new_asset_name[0] = '\0';
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();

	if (!is_open)
		return;

	// List assets of this type
	ImGui::Indent();
	bool any_rendered = false;

	for (const auto& asset : assets.GetAll()) {
		if (!asset || asset->type != type_info.asset_type) {
			continue;
		}
		any_rendered = true;

		// Highlight if selected
		const bool is_selected = selected_asset.IsValid() && selected_asset.type == type_info.asset_type
								 && selected_asset.name == asset->name;

		ImGuiTreeNodeFlags node_flags =
				ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (is_selected) {
			node_flags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::PushID(asset->name.c_str());
		ImGui::TreeNodeEx("##node", node_flags, "%s", asset->name.c_str());

		// Click to select
		if (ImGui::IsItemClicked()) {
			if (callbacks_.on_asset_selected) {
				callbacks_.on_asset_selected(type_info.asset_type, asset->name);
			}
		}

		// Context menu for individual asset
		if (ImGui::BeginPopupContextItem("AssetContextMenu")) {
			if (ImGui::MenuItem("Delete")) {
				const std::string name_copy = asset->name;
				if (assets.Remove(asset->name, type_info.asset_type)) {
					if (callbacks_.on_asset_deleted) {
						callbacks_.on_asset_deleted(type_info.asset_type, name_copy);
					}
					if (callbacks_.on_scene_modified) {
						callbacks_.on_scene_modified();
					}
				}
				ImGui::EndPopup();
				ImGui::PopID();
				ImGui::Unindent();
				return; // Exit early as we modified the collection
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	if (!any_rendered) {
		ImGui::TextDisabled("No %s assets", type_info.display_name.c_str());
	}

	ImGui::Unindent();
}

void AssetBrowserPanel::RenderPrefabSection() {
	if (!prefabs_scanned_) {
		ScanForPrefabs();
	}

	const std::string header_label = "Prefabs (" + std::to_string(prefab_files_.size()) + ")";

	const bool is_open = ImGui::CollapsingHeader(
			header_label.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

	// Refresh button
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	if (ImGui::SmallButton("R##refresh_prefabs")) {
		ScanForPrefabs();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Rescan for prefab files");
	}

	if (!is_open) {
		return;
	}

	ImGui::Indent();

	if (prefab_files_.empty()) {
		ImGui::TextDisabled("No prefab files found");
		ImGui::TextDisabled("Right-click an entity > Save as Prefab");
	}
	else {
		for (const auto& prefab_path : prefab_files_) {
			// Extract display name from path
			const std::filesystem::path path(prefab_path);
			std::string display_name = path.stem().string();
			// Remove .prefab suffix if present (file is name.prefab.json)
			if (display_name.ends_with(".prefab")) {
				display_name = display_name.substr(0, display_name.size() - 7);
			}

			const ImGuiTreeNodeFlags node_flags =
					ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

			ImGui::PushID(prefab_path.c_str());
			ImGui::TreeNodeEx("##prefab_node", node_flags, "[P] %s", display_name.c_str());

			// Right-click context menu
			if (ImGui::BeginPopupContextItem("PrefabContextMenu")) {
				if (ImGui::MenuItem("Instantiate")) {
					if (callbacks_.on_instantiate_prefab) {
						callbacks_.on_instantiate_prefab(prefab_path);
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Delete File")) {
					std::filesystem::remove(prefab_path);
					prefabs_scanned_ = false; // Trigger rescan
				}
				ImGui::EndPopup();
			}

			// Double-click to instantiate
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (callbacks_.on_instantiate_prefab) {
					callbacks_.on_instantiate_prefab(prefab_path);
				}
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", prefab_path.c_str());
			}

			ImGui::PopID();
		}
	}

	ImGui::Unindent();
}

void AssetBrowserPanel::ScanForPrefabs() {
	prefab_files_.clear();
	prefabs_scanned_ = true;

	// Scan the assets directory for .prefab.json files
	const std::filesystem::path assets_dir("assets");
	if (!std::filesystem::exists(assets_dir)) {
		return;
	}

	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_dir)) {
			if (entry.is_regular_file()) {
				const auto& path = entry.path();
				const std::string filename = path.filename().string();
				if (filename.ends_with(".prefab.json")) {
					prefab_files_.push_back(path.string());
				}
			}
		}
		// Also check the current directory
		if (std::filesystem::exists(".")) {
			for (const auto& entry : std::filesystem::directory_iterator(".")) {
				if (entry.is_regular_file()) {
					const auto& path = entry.path();
					const std::string filename = path.filename().string();
					if (filename.ends_with(".prefab.json")) {
						prefab_files_.push_back(path.string());
					}
				}
			}
		}
	}
	catch (const std::exception&) {
		// Silently handle filesystem errors
	}
}

} // namespace editor
