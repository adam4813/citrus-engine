#include "asset_browser_panel.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>

namespace editor {

std::string_view AssetBrowserPanel::GetPanelName() const { return "Assets"; }

void AssetBrowserPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

std::string AssetBrowserPanel::GetFileIcon(const std::filesystem::path& path) {
	if (std::filesystem::is_directory(path)) {
		return "[D]";
	}

	const auto ext = path.extension().string();
	if (ext == ".scene" || path.filename().string().ends_with(".scene.json")) {
		return "[Sc]";
	}
	if (ext == ".prefab" || path.filename().string().ends_with(".prefab.json")) {
		return "[P]";
	}
	if (path.filename().string().ends_with(".tileset.json")) {
		return "[TS]";
	}
	if (path.filename().string().ends_with(".data.json")) {
		return "[Dt]";
	}
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
		return "[T]";
	}
	if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
		return "[S]";
	}
	if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
		return "[M]";
	}
	if (ext == ".lua" || ext == ".as" || ext == ".js") {
		return "[Sc]";
	}
	if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
		return "[Sh]";
	}
	if (ext == ".json") {
		return "[J]";
	}
	return "[F]";
}

AssetFileType AssetBrowserPanel::GetAssetFileType(const std::filesystem::path& path) {
	if (std::filesystem::is_directory(path)) {
		return AssetFileType::Directory;
	}

	const auto ext = path.extension().string();
	const auto filename = path.filename().string();

	if (ext == ".scene" || filename.ends_with(".scene.json")) {
		return AssetFileType::Scene;
	}
	if (ext == ".prefab" || filename.ends_with(".prefab.json")) {
		return AssetFileType::Prefab;
	}
	if (filename.ends_with(".data.json")) {
		return AssetFileType::DataTable;
	}
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
		return AssetFileType::Texture;
	}
	if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
		return AssetFileType::Sound;
	}
	if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
		return AssetFileType::Mesh;
	}
	if (ext == ".lua" || ext == ".as" || ext == ".js") {
		return AssetFileType::Script;
	}
	if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
		return AssetFileType::Shader;
	}

	return AssetFileType::All;
}

bool AssetBrowserPanel::PassesFilter(const FileSystemItem& item) const {
	// Search filter
	if (search_buffer_[0] != '\0') {
		const std::string search_lower = [this]() {
			std::string s = search_buffer_;
			std::transform(s.begin(), s.end(), s.begin(), ::tolower);
			return s;
		}();

		std::string name_lower = item.display_name;
		std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

		if (name_lower.find(search_lower) == std::string::npos) {
			return false;
		}
	}

	// Type filter
	if (filter_type_ != AssetFileType::All) {
		const auto item_type = GetAssetFileType(item.path);
		if (item_type != filter_type_ && item_type != AssetFileType::Directory) {
			return false;
		}
	}

	return true;
}

void AssetBrowserPanel::RefreshCurrentDirectory() {
	current_items_.clear();

	if (!std::filesystem::exists(current_directory_)) {
		needs_refresh_ = false;
		return;
	}

	try {
		for (const auto& entry : std::filesystem::directory_iterator(current_directory_)) {
			FileSystemItem item(entry.path(), entry.is_directory());
			item.type_icon = GetFileIcon(entry.path());
			current_items_.push_back(item);
		}

		// Sort: directories first, then alphabetically
		std::sort(current_items_.begin(), current_items_.end(), [](const auto& a, const auto& b) {
			if (a.is_directory != b.is_directory) {
				return a.is_directory > b.is_directory;
			}
			return a.display_name < b.display_name;
		});
	}
	catch (const std::filesystem::filesystem_error&) {
		// Silently handle errors
	}

	needs_refresh_ = false;
}

void AssetBrowserPanel::Render(engine::scene::Scene* scene, const AssetSelection& selected_asset) {
	if (!IsVisible())
		return;

	if (needs_refresh_) {
		RefreshCurrentDirectory();
	}

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
										 | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Assets", &VisibleRef());

	if (!scene) {
		ImGui::TextDisabled("No scene loaded");
		ImGui::End();
		return;
	}

	// Breadcrumb navigation
	RenderBreadcrumbBar();

	ImGui::Separator();

	// Search and filter bar
	RenderSearchBar();

	ImGui::Separator();

	// Two-panel layout: directory tree (left) and content view (right)
	const float tree_width = 200.0f;

	// Left panel: Directory tree
	ImGui::BeginChild("DirectoryTree", ImVec2(tree_width, 0), true);
	RenderDirectoryTree();
	ImGui::EndChild();

	ImGui::SameLine();

	// Right panel: Content view + Scene assets
	ImGui::BeginChild("ContentView", ImVec2(0, 0), true);
	RenderContentView();

	// Scene-embedded assets section
	if (scene && !scene->GetAssets().GetAll().empty()) {
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Scene Assets", ImGuiTreeNodeFlags_DefaultOpen)) {
			auto& registry = engine::scene::AssetRegistry::Instance();
			for (const auto& type_info : registry.GetAssetTypes()) {
				RenderAssetCategory(scene, type_info, selected_asset);
			}
		}
	}

	// Handle right-click on empty space
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("EmptySpaceContextMenu");
	}
	ShowEmptySpaceContextMenu();

	ImGui::EndChild();

	ImGui::End();
}

void AssetBrowserPanel::RenderBreadcrumbBar() {
	ImGui::Text("Path:");
	ImGui::SameLine();

	// Show clickable breadcrumb trail
	std::filesystem::path path = current_directory_;
	std::vector<std::filesystem::path> segments;

	// Build segments from assets root
	for (auto it = current_directory_; it != assets_root_.parent_path(); it = it.parent_path()) {
		segments.push_back(it);
		if (it == assets_root_)
			break;
	}
	std::reverse(segments.begin(), segments.end());

	for (size_t i = 0; i < segments.size(); ++i) {
		if (i > 0) {
			ImGui::SameLine();
			ImGui::Text("/");
			ImGui::SameLine();
		}

		const auto& segment = segments[i];
		const std::string label = segment.filename().string();

		if (ImGui::Button(label.c_str())) {
			current_directory_ = segment;
			needs_refresh_ = true;
		}
	}

	// View mode toggle
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);

	if (view_mode_ == AssetViewMode::Grid) {
		if (ImGui::Button("Grid View")) {
			view_mode_ = AssetViewMode::List;
		}
	}
	else {
		if (ImGui::Button("List View")) {
			view_mode_ = AssetViewMode::Grid;
		}
	}
}

void AssetBrowserPanel::RenderSearchBar() {
	ImGui::Text("Search:");
	ImGui::SameLine();

	ImGui::SetNextItemWidth(200);
	if (ImGui::InputText("##search", search_buffer_, sizeof(search_buffer_))) {
		// Filter updated
	}

	ImGui::SameLine();
	ImGui::Text("Filter:");
	ImGui::SameLine();

	ImGui::SetNextItemWidth(120);
	const char* filter_items[] = {"All", "Scene", "Prefab", "Texture", "Sound", "Mesh", "Script", "Shader", "Data"};
	int current_filter = static_cast<int>(filter_type_);
	if (ImGui::Combo("##filter", &current_filter, filter_items, IM_ARRAYSIZE(filter_items))) {
		filter_type_ = static_cast<AssetFileType>(current_filter);
	}

	ImGui::SameLine();
	if (ImGui::Button("Refresh")) {
		needs_refresh_ = true;
	}
}

void AssetBrowserPanel::RenderDirectoryTree() {
	if (!std::filesystem::exists(assets_root_)) {
		ImGui::TextDisabled("Assets folder not found");
		return;
	}

	RenderDirectoryTreeNode(assets_root_);
}

void AssetBrowserPanel::RenderDirectoryTreeNode(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
		return;
	}

	const std::string name = path.filename().string();
	const bool is_selected = (path == current_directory_);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (is_selected) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// Check if this directory has subdirectories
	bool has_subdirs = false;
	try {
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_directory()) {
				has_subdirs = true;
				break;
			}
		}
	}
	catch (...) {
		// Ignore errors
	}

	if (!has_subdirs) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	const bool node_open = ImGui::TreeNodeEx(name.c_str(), flags);

	if (ImGui::IsItemClicked()) {
		current_directory_ = path;
		needs_refresh_ = true;
	}

	if (node_open) {
		if (has_subdirs) {
			try {
				// Collect and sort subdirectories
				std::vector<std::filesystem::path> subdirs;
				for (const auto& entry : std::filesystem::directory_iterator(path)) {
					if (entry.is_directory()) {
						subdirs.push_back(entry.path());
					}
				}
				std::sort(subdirs.begin(), subdirs.end());

				for (const auto& subdir : subdirs) {
					RenderDirectoryTreeNode(subdir);
				}
			}
			catch (...) {
				// Ignore errors
			}
		}
		ImGui::TreePop();
	}
}

void AssetBrowserPanel::RenderContentView() {
	if (current_items_.empty()) {
		ImGui::TextDisabled("Empty directory");
		return;
	}

	if (view_mode_ == AssetViewMode::Grid) {
		// Grid view with icons
		const float item_size = 80.0f;
		const float padding = 10.0f;
		const float window_width = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((window_width + padding) / (item_size + padding)));

		int col = 0;
		for (const auto& item : current_items_) {
			if (!PassesFilter(item)) {
				continue;
			}

			ImGui::PushID(item.path.string().c_str());

			if (col > 0) {
				ImGui::SameLine();
			}

			RenderAssetItemGrid(item);

			col++;
			if (col >= columns) {
				col = 0;
			}

			ImGui::PopID();
		}
	}
	else {
		// List view
		if (ImGui::BeginTable("AssetList", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableHeadersRow();

			for (const auto& item : current_items_) {
				if (!PassesFilter(item)) {
					continue;
				}

				ImGui::PushID(item.path.string().c_str());
				RenderAssetItemList(item);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}
}

void AssetBrowserPanel::RenderAssetItemList(const FileSystemItem& item) {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	const bool is_selected = (item.path == selected_item_path_);
	if (ImGui::Selectable(item.display_name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
		if (item.is_directory) {
			current_directory_ = item.path;
			needs_refresh_ = true;
		}
		else {
			selected_item_path_ = item.path;
			if (callbacks_.on_file_selected) {
				callbacks_.on_file_selected(item.path.string());
			}
			// Handle double-click actions
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (callbacks_.on_open_asset_file) {
					callbacks_.on_open_asset_file(item.path.string());
				}
			}
		}
	}

	// Context menu
	if (ImGui::BeginPopupContextItem("ItemContextMenu")) {
		ShowItemContextMenu(item);
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();
	ImGui::Text("%s", item.type_icon.c_str());
}

void AssetBrowserPanel::RenderAssetItemGrid(const FileSystemItem& item) {
	const float item_size = 80.0f;
	const bool is_selected = (item.path == selected_item_path_);

	ImVec2 button_size(item_size, item_size);

	ImGui::BeginGroup();

	if (is_selected) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}

	if (ImGui::Button(item.type_icon.c_str(), button_size)) {
		if (item.is_directory) {
			current_directory_ = item.path;
			needs_refresh_ = true;
		}
		else {
			selected_item_path_ = item.path;
			if (callbacks_.on_file_selected) {
				callbacks_.on_file_selected(item.path.string());
			}
		}
	}

	if (is_selected) {
		ImGui::PopStyleColor();
	}

	// Double-click handling
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		if (item.is_directory) {
			current_directory_ = item.path;
			needs_refresh_ = true;
		}
		else if (callbacks_.on_open_asset_file) {
			callbacks_.on_open_asset_file(item.path.string());
		}
	}

	// Context menu
	if (ImGui::BeginPopupContextItem("GridItemContextMenu")) {
		ShowItemContextMenu(item);
		ImGui::EndPopup();
	}

	// Display name below icon (truncated to cell width)
	const std::string& name = item.display_name;
	const float text_width = ImGui::CalcTextSize(name.c_str()).x;
	if (text_width > item_size) {
		// Truncate with ellipsis
		std::string truncated;
		for (size_t i = 0; i < name.size(); ++i) {
			const std::string candidate = name.substr(0, i) + "...";
			if (ImGui::CalcTextSize(candidate.c_str()).x > item_size) {
				break;
			}
			truncated = candidate;
		}
		ImGui::TextUnformatted(truncated.c_str());
	}
	else {
		// Center the text
		const float offset = (item_size - text_width) * 0.5f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
		ImGui::TextUnformatted(name.c_str());
	}

	ImGui::EndGroup();

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", item.path.string().c_str());
	}
}

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
		// TODO: Show rename dialog
	}

	if (ImGui::MenuItem("Delete")) {
		try {
			std::filesystem::remove(item.path);
			needs_refresh_ = true;
		}
		catch (...) {
			// Handle error
		}
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Copy Path")) {
		ImGui::SetClipboardText(item.path.string().c_str());
	}

#ifdef _WIN32
	if (ImGui::MenuItem("Show in Explorer")) {
		const std::string command = "explorer /select,\"" + item.path.string() + "\"";
		system(command.c_str());
	}
#elif defined(__linux__)
	if (ImGui::MenuItem("Show in File Manager")) {
		const std::string command = "xdg-open \"" + item.path.parent_path().string() + "\"";
		system(command.c_str());
	}
#elif defined(__APPLE__)
	if (ImGui::MenuItem("Show in Finder")) {
		const std::string command = "open -R \"" + item.path.string() + "\"";
		system(command.c_str());
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
			// TODO: Create new scene file
		}

		if (ImGui::MenuItem("New Prefab")) {
			// TODO: Create new prefab file
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Import Asset...")) {
			// TODO: Open file dialog for import
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Refresh")) {
			needs_refresh_ = true;
		}

		ImGui::EndPopup();
	}
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

			// Single-click to select and show properties
			if (ImGui::IsItemClicked()) {
				if (callbacks_.on_file_selected) {
					callbacks_.on_file_selected(prefab_path);
				}
			}

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
