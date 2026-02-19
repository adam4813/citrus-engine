#include "asset_browser_panel.h"
#include "file_dialog.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <nlohmann/json.hpp>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef _WIN32
#include <windows.h>

#include <shellapi.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#endif

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

AssetBrowserPanel::~AssetBrowserPanel() { ClearThumbnailCache(); }

void AssetBrowserPanel::ClearThumbnailCache() {
	for (const auto& [path, tex_id] : thumbnail_cache_) {
		if (tex_id != 0) {
			glDeleteTextures(1, &tex_id);
		}
	}
	thumbnail_cache_.clear();
}

uint32_t AssetBrowserPanel::GetOrLoadThumbnail(const std::filesystem::path& path) {
	const std::string key = path.string();
	if (const auto it = thumbnail_cache_.find(key); it != thumbnail_cache_.end()) {
		return it->second;
	}

	// Load image with stb_image
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
	if (!pixels) {
		thumbnail_cache_[key] = 0;
		return 0;
	}

	// Downscale to thumbnail size if needed
	constexpr int MAX_THUMB = 128;
	int thumb_w = width;
	int thumb_h = height;
	std::vector<unsigned char> thumb_data;

	if (width > MAX_THUMB || height > MAX_THUMB) {
		const float scale = static_cast<float>(MAX_THUMB) / static_cast<float>(std::max(width, height));
		thumb_w = std::max(1, static_cast<int>(static_cast<float>(width) * scale));
		thumb_h = std::max(1, static_cast<int>(static_cast<float>(height) * scale));
		thumb_data.resize(static_cast<size_t>(thumb_w) * thumb_h * 4);
		for (int y = 0; y < thumb_h; ++y) {
			for (int x = 0; x < thumb_w; ++x) {
				const int src_x = x * width / thumb_w;
				const int src_y = y * height / thumb_h;
				const int src_idx = (src_y * width + src_x) * 4;
				const int dst_idx = (y * thumb_w + x) * 4;
				std::memcpy(&thumb_data[dst_idx], &pixels[src_idx], 4);
			}
		}
	}

	const unsigned char* tex_pixels = thumb_data.empty() ? pixels : thumb_data.data();

	// Create GL texture
	GLuint tex_id = 0;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thumb_w, thumb_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);

	thumbnail_cache_[key] = tex_id;
	return tex_id;
}

ImVec4 AssetBrowserPanel::GetAssetTypeColor(AssetFileType type) {
	switch (type) {
		case AssetFileType::Scene: return {0.3f, 0.85f, 0.4f, 1.0f};
		case AssetFileType::Prefab: return {0.4f, 0.6f, 1.0f, 1.0f};
		case AssetFileType::Texture: return {0.95f, 0.75f, 0.2f, 1.0f};
		case AssetFileType::Sound: return {1.0f, 0.55f, 0.2f, 1.0f};
		case AssetFileType::Mesh: return {0.7f, 0.4f, 0.9f, 1.0f};
		case AssetFileType::Script: return {0.3f, 0.8f, 0.8f, 1.0f};
		case AssetFileType::Shader: return {0.9f, 0.4f, 0.7f, 1.0f};
		case AssetFileType::DataTable: return {0.65f, 0.65f, 0.65f, 1.0f};
		case AssetFileType::Directory: return {0.95f, 0.85f, 0.3f, 1.0f};
		default: return {0.55f, 0.55f, 0.55f, 1.0f};
	}
}

void AssetBrowserPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

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
	ClearThumbnailCache();

	if (!std::filesystem::exists(current_directory_)) {
		needs_refresh_ = false;
		return;
	}

	try {
		for (const auto& entry : std::filesystem::directory_iterator(current_directory_)) {
			FileSystemItem item(entry.path(), entry.is_directory());
			item.type_icon = GetFileIcon(entry);
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
	const ImVec4 tc = GetAssetTypeColor(GetAssetFileType(item.path));
	ImGui::TextColored(tc, "%s", item.type_icon.c_str());
}

void AssetBrowserPanel::RenderAssetItemGrid(const FileSystemItem& item) {
	const float cell_width = ImGui::GetContentRegionAvail().x;
	const float icon_size = 64.0f;
	const bool is_selected = (item.path == selected_item_path_);
	const auto file_type = GetAssetFileType(item.path);

	ImGui::BeginGroup();

	// Center the icon button within the cell
	const float icon_offset = std::max(0.0f, (cell_width - icon_size) * 0.5f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + icon_offset);

	bool clicked = false;

	// For texture files, try to show an image thumbnail
	if (file_type == AssetFileType::Texture) {
		const uint32_t thumb_id = GetOrLoadThumbnail(item.path);
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
			// Fallback: colored text button
			const ImVec4 tc = GetAssetTypeColor(file_type);
			ImGui::PushStyleColor(
					ImGuiCol_Button, is_selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
												 : ImVec4(tc.x * 0.25f, tc.y * 0.25f, tc.z * 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(tc.x * 0.4f, tc.y * 0.4f, tc.z * 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, tc);
			clicked = ImGui::Button(item.type_icon.c_str(), ImVec2(icon_size, icon_size));
			ImGui::PopStyleColor(3);
		}
	}
	else {
		// Colored button for non-image asset types
		const ImVec4 tc = GetAssetTypeColor(file_type);
		ImGui::PushStyleColor(
				ImGuiCol_Button, is_selected ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
											 : ImVec4(tc.x * 0.25f, tc.y * 0.25f, tc.z * 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(tc.x * 0.4f, tc.y * 0.4f, tc.z * 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, tc);
		clicked = ImGui::Button(item.type_icon.c_str(), ImVec2(icon_size, icon_size));
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

void AssetBrowserPanel::ShowImportAssetDialog() {
	if (import_dialog_) {
		import_dialog_->Open();
	}
}

} // namespace editor
