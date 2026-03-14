#include "asset_browser_panel.h"
#include "file_dialog.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

namespace editor {

AssetBrowserPanel::AssetBrowserPanel() {
	// Initialize import dialog
	import_dialog_ = std::make_unique<FileDialogPopup>("Import Asset", FileDialogMode::Open);
	import_dialog_->SetRoot(std::filesystem::current_path()); // Start from project root, not assets
	import_dialog_->SetCallback([this](const std::string& source_path) {
		try {
			const std::filesystem::path source(source_path);
			const std::filesystem::path dest = current_directory_ / source.filename();

			// Copy the file to the current directory
			std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing);

			needs_refresh_ = true;
			selected_item_path_ = dest;
			std::cout << "Imported asset: " << source << " -> " << dest << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to import asset: " << e.what() << std::endl;
		}
	});
}

std::string_view AssetBrowserPanel::GetPanelName() const { return "Assets"; }

AssetBrowserPanel::~AssetBrowserPanel() { AssetPreviewRegistry::Instance().ClearCache(); }

void AssetBrowserPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

namespace {

AssetFileType GetAssetFileType(const std::filesystem::path& path) {
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
	if (filename.ends_with(".material.json")) {
		return AssetFileType::Material;
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

} // namespace

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

	// Handle right-click on empty space
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("EmptySpaceContextMenu");
	}
	ShowEmptySpaceContextMenu();

	ImGui::EndChild();

	// Render rename dialog if active
	RenderRenameDialog();

	// Render delete confirmation dialog if active
	RenderDeleteConfirmationDialog();

	// Render import dialog if active
	if (import_dialog_) {
		import_dialog_->Render();
	}

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

	if (std::filesystem::path new_dir; editor::RenderDirectoryTree(assets_root_, current_directory_, new_dir, false)) {
		current_directory_ = new_dir;
		needs_refresh_ = true;
	}
}

void AssetBrowserPanel::RenderContentView() {
	if (current_items_.empty()) {
		ImGui::TextDisabled("Empty directory");
		return;
	}

	if (view_mode_ == AssetViewMode::Grid) {
		// Grid view with uniform cells
		const float cell_size = 90.0f;
		const float padding = 8.0f;
		const float window_width = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>(window_width / (cell_size + padding)));

		if (ImGui::BeginTable("GridView", columns)) {
			for (int c = 0; c < columns; ++c) {
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, cell_size);
			}

			int col = 0;
			for (const auto& item : current_items_) {
				if (!PassesFilter(item)) {
					continue;
				}

				if (col == 0) {
					ImGui::TableNextRow();
				}
				ImGui::TableSetColumnIndex(col);

				ImGui::PushID(item.path.string().c_str());
				RenderAssetItemGrid(item);
				ImGui::PopID();

				col++;
				if (col >= columns) {
					col = 0;
				}
			}

			ImGui::EndTable();
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
	const auto list_appearance = AssetPreviewRegistry::Instance().GetAppearance(item.path);
	ImGui::TextColored(list_appearance.color, "%s",
			item.type_icon.empty() ? list_appearance.icon.c_str() : item.type_icon.c_str());
}

void AssetBrowserPanel::RenderAssetItemGrid(const FileSystemItem& item) {
	const float cell_width = ImGui::GetContentRegionAvail().x;
	const float icon_size = 64.0f;
	const bool is_selected = (item.path == selected_item_path_);

	auto& preview_registry = AssetPreviewRegistry::Instance();
	const auto appearance = preview_registry.GetAppearance(item.path);

	ImGui::BeginGroup();

	// Center the icon button within the cell
	const float icon_offset = std::max(0.0f, (cell_width - icon_size) * 0.5f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + icon_offset);

	bool clicked = false;

	// Try to show a thumbnail if a preview generator is registered
	const uint32_t thumb_id =
			!item.is_directory ? preview_registry.GetOrGeneratePreview(item.path) : 0;
	if (thumb_id != 0) {
		if (is_selected) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(thumb_id));
		clicked = ImGui::ImageButton("##thumb", imgui_tex, ImVec2(icon_size, icon_size));
		if (is_selected) {
			ImGui::PopStyleColor();
		}
	}
	else {
		// Colored icon button from appearance registry
		const ImVec4& tc = appearance.color;
		ImGui::PushStyleColor(
				ImGuiCol_Button,
				is_selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
							: ImVec4(tc.x * 0.25f, tc.y * 0.25f, tc.z * 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(tc.x * 0.4f, tc.y * 0.4f, tc.z * 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, tc);
		const std::string& icon =
				item.type_icon.empty() ? appearance.icon : item.type_icon;
		clicked = ImGui::Button(icon.c_str(), ImVec2(icon_size, icon_size));
		ImGui::PopStyleColor(3);
	}

	if (clicked) {
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
	if (text_width > cell_width) {
		std::string truncated;
		for (size_t i = 0; i < name.size(); ++i) {
			const std::string candidate = name.substr(0, i) + "...";
			if (ImGui::CalcTextSize(candidate.c_str()).x > cell_width) {
				break;
			}
			truncated = candidate;
		}
		ImGui::TextUnformatted(truncated.c_str());
	}
	else {
		const float offset = std::max(0.0f, (cell_width - text_width) * 0.5f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
		ImGui::TextUnformatted(name.c_str());
	}

	ImGui::EndGroup();

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", item.path.string().c_str());
	}
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

} // namespace editor
