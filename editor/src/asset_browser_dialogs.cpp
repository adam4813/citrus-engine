#include "asset_browser_panel.h"
#include "file_dialog.h"

#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>

#include <shellapi.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace editor {

void AssetBrowserPanel::ShowItemContextMenu(const FileSystemItem& item) {
	if (item.is_directory) {
		if (ImGui::MenuItem("Open")) {
			current_directory_ = item.path;
			needs_refresh_ = true;
		}
		ImGui::Separator();
	}
	else {
		// File-specific actions
		if (item.path.filename().string().ends_with(".prefab.json")) {
			if (ImGui::MenuItem("Instantiate")) {
				if (callbacks_.on_instantiate_prefab) {
					callbacks_.on_instantiate_prefab(item.path.string());
				}
			}
			ImGui::Separator();
		}
	}

	if (ImGui::MenuItem("Rename")) {
		// Open rename dialog
		show_rename_dialog_ = true;
		rename_target_path_ = item.path;
		std::strncpy(rename_buffer_, item.display_name.c_str(), sizeof(rename_buffer_) - 1);
		rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
	}

	if (ImGui::MenuItem("Delete")) {
		// Set flag to open confirmation dialog at window level
		delete_target_path_ = item.path;
		pending_delete_ = true;
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Copy Path")) {
		ImGui::SetClipboardText(item.path.string().c_str());
	}

#ifdef _WIN32
	if (ImGui::MenuItem("Show in Explorer")) {
		const std::wstring wide_path = item.path.wstring();
		ShellExecuteW(
				nullptr, L"open", L"explorer.exe", (L"/select,\"" + wide_path + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
	}
#elif defined(__linux__)
	if (ImGui::MenuItem("Show in File Manager")) {
		const std::string dir_path = item.path.parent_path().string();
		pid_t pid = fork();
		if (pid == 0) {
			// Child process
			execlp("xdg-open", "xdg-open", dir_path.c_str(), nullptr);
			_exit(1); // If exec fails
		}
	}
#elif defined(__APPLE__)
	if (ImGui::MenuItem("Show in Finder")) {
		const std::string file_path = item.path.string();
		pid_t pid = fork();
		if (pid == 0) {
			// Child process
			execlp("open", "open", "-R", file_path.c_str(), nullptr);
			_exit(1); // If exec fails
		}
	}
#endif
}

void AssetBrowserPanel::ShowEmptySpaceContextMenu() {
	if (ImGui::BeginPopup("EmptySpaceContextMenu")) {
		if (ImGui::MenuItem("New Folder")) {
			try {
				auto new_folder = current_directory_ / "NewFolder";
				int counter = 1;
				while (std::filesystem::exists(new_folder)) {
					new_folder = current_directory_ / ("NewFolder" + std::to_string(counter++));
				}
				std::filesystem::create_directory(new_folder);
				needs_refresh_ = true;
			}
			catch (...) {
				// Handle error
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("New Scene")) {
			CreateNewSceneFile();
		}

		if (ImGui::MenuItem("New Prefab")) {
			CreateNewPrefabFile();
		}

		if (ImGui::MenuItem("New Material")) {
			CreateNewMaterialFile();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Import Asset...")) {
			ShowImportAssetDialog();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Refresh")) {
			needs_refresh_ = true;
		}

		ImGui::EndPopup();
	}
}

void AssetBrowserPanel::RenderRenameDialog() {
	if (show_rename_dialog_) {
		ImGui::OpenPopup("Rename Asset");
		show_rename_dialog_ = false;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Rename Asset", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::Text("Rename: %s", rename_target_path_.filename().string().c_str());
		ImGui::Separator();

		ImGui::Text("New name:");
		ImGui::SetNextItemWidth(-1);
		const bool enter_pressed = ImGui::InputText(
				"##rename_input", rename_buffer_, sizeof(rename_buffer_), ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::Separator();

		if ((ImGui::Button("Rename", ImVec2(120, 0)) || enter_pressed) && rename_buffer_[0] != '\0') {
			try {
				const auto new_path = rename_target_path_.parent_path() / rename_buffer_;
				if (new_path != rename_target_path_) {
					std::filesystem::rename(rename_target_path_, new_path);
					needs_refresh_ = true;
					if (selected_item_path_ == rename_target_path_) {
						selected_item_path_ = new_path;
					}
				}
				ImGui::CloseCurrentPopup();
			}
			catch (const std::exception& e) {
				// TODO: Show error message to user
				std::cerr << "Failed to rename: " << e.what() << std::endl;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AssetBrowserPanel::RenderDeleteConfirmationDialog() {
	if (pending_delete_) {
		ImGui::OpenPopup("DeleteConfirmation");
		pending_delete_ = false;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("DeleteConfirmation", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::Text("Are you sure you want to delete this file?");
		ImGui::Separator();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", delete_target_path_.filename().string().c_str());

		ImGui::Separator();
		ImGui::Text("This action cannot be undone.");
		ImGui::Separator();

		if (ImGui::Button("Delete", ImVec2(120, 0))) {
			try {
				std::filesystem::remove(delete_target_path_);
				needs_refresh_ = true;
				if (selected_item_path_ == delete_target_path_) {
					selected_item_path_.clear();
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Failed to delete: " << e.what() << std::endl;
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AssetBrowserPanel::CreateNewSceneFile() {
	using json = nlohmann::json;

	try {
		// Generate a unique scene filename
		auto new_scene = current_directory_ / "NewScene.scene.json";
		int counter = 1;
		while (std::filesystem::exists(new_scene)) {
			new_scene = current_directory_ / ("NewScene" + std::to_string(counter++) + ".scene.json");
		}

		// Create default scene JSON content
		json scene_doc;
		scene_doc["version"] = 1;
		scene_doc["name"] = new_scene.stem().stem().string(); // Remove .scene.json to get base name

		// Metadata
		json metadata;
		metadata["engine_version"] = "0.0.9";
		scene_doc["metadata"] = metadata;

		// Scene settings
		json settings;
		settings["background_color"] = {0.1f, 0.1f, 0.1f, 1.0f};
		settings["ambient_light"] = {0.3f, 0.3f, 0.3f, 1.0f};
		settings["physics_backend"] = "none";
		settings["author"] = "";
		settings["description"] = "";
		scene_doc["settings"] = settings;

		// Empty assets array
		scene_doc["assets"] = json::array();

		// Empty flecs data (empty world)
		scene_doc["flecs_data"] = "{}";

		// Write to file using AssetManager (use absolute path overload to avoid double nesting)
		const std::string json_str = scene_doc.dump(2); // Pretty print with 2-space indent

		if (engine::assets::AssetManager::SaveTextFile(new_scene, json_str)) {
			needs_refresh_ = true;
			selected_item_path_ = new_scene;
			std::cout << "Created new scene: " << new_scene << std::endl;
		}
		else {
			std::cerr << "Failed to create scene file: " << new_scene << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating scene: " << e.what() << std::endl;
	}
}

void AssetBrowserPanel::CreateNewPrefabFile() {
	using json = nlohmann::json;

	try {
		// Generate a unique prefab filename
		auto new_prefab = current_directory_ / "NewPrefab.prefab.json";
		int counter = 1;
		while (std::filesystem::exists(new_prefab)) {
			new_prefab = current_directory_ / ("NewPrefab" + std::to_string(counter++) + ".prefab.json");
		}

		// Create default prefab JSON content
		json prefab_doc;
		prefab_doc["version"] = 1;
		prefab_doc["name"] = new_prefab.stem().stem().string(); // Remove .prefab.json to get base name

		// Empty entity data (single entity with just a name)
		json entity_data;
		entity_data["name"] = prefab_doc["name"];
		entity_data["components"] = json::object();

		prefab_doc["entity_data"] = entity_data.dump(); // Store as JSON string

		// Write to file using AssetManager (use absolute path overload to avoid double nesting)
		const std::string json_str = prefab_doc.dump(2); // Pretty print with 2-space indent

		if (engine::assets::AssetManager::SaveTextFile(new_prefab, json_str)) {
			needs_refresh_ = true;
			selected_item_path_ = new_prefab;
			prefabs_scanned_ = false; // Trigger prefab rescan
			std::cout << "Created new prefab: " << new_prefab << std::endl;
		}
		else {
			std::cerr << "Failed to create prefab file: " << new_prefab << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating prefab: " << e.what() << std::endl;
	}
}

void AssetBrowserPanel::CreateNewMaterialFile() {
	try {
		auto new_mat = current_directory_ / "NewMaterial.material.json";
		int counter = 1;
		while (std::filesystem::exists(new_mat)) {
			new_mat = current_directory_ / ("NewMaterial" + std::to_string(counter++) + ".material.json");
		}

		// Use the asset registry to create a default material and serialize it
		auto default_asset =
				engine::assets::AssetRegistry::Instance().CreateDefault(engine::assets::AssetType::MATERIAL);
		if (!default_asset) {
			std::cerr << "Failed to create default material from registry" << std::endl;
			return;
		}

		// Set name from filename
		std::string base_name = new_mat.stem().string();
		if (base_name.ends_with(".material")) {
			base_name = base_name.substr(0, base_name.size() - 9);
		}
		default_asset->name = base_name;

		nlohmann::json j;
		default_asset->ToJson(j);
		const std::string json_str = j.dump(2);

		if (engine::assets::AssetManager::SaveTextFile(new_mat, json_str)) {
			needs_refresh_ = true;
			selected_item_path_ = new_mat;
			std::cout << "Created new material: " << new_mat << std::endl;
		}
		else {
			std::cerr << "Failed to create material file: " << new_mat << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating material: " << e.what() << std::endl;
	}
}

void AssetBrowserPanel::ShowImportAssetDialog() {
	if (import_dialog_) {
		import_dialog_->Open();
	}
}

} // namespace editor
