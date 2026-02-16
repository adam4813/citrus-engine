#include "tileset_editor_panel.h"
#include "asset_editor_registry.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>

using json = nlohmann::json;

namespace editor {

TilesetEditorPanel::TilesetEditorPanel() : tileset_(std::make_unique<TilesetDefinition>()) {
	open_dialog_.SetCallback([this](const std::string& path) { OpenTileset(path); });
	save_dialog_.SetCallback([this](const std::string& path) { SaveTileset(path); });
	image_dialog_.SetCallback([this](const std::string& path) {
		std::strncpy(image_path_buffer_, path.c_str(), sizeof(image_path_buffer_) - 1);
		image_path_buffer_[sizeof(image_path_buffer_) - 1] = '\0';
		pending_image_load_ = true;
		tileset_->source_image_path = path;
	});
}

std::string_view TilesetEditorPanel::GetPanelName() const { return "Tileset Editor"; }

void TilesetEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("tileset", [this](const std::string& path) { OpenTileset(path); });
}

TilesetEditorPanel::~TilesetEditorPanel() = default;

void TilesetEditorPanel::Render(engine::Engine& engine) {
	if (!BeginPanel(ImGuiWindowFlags_MenuBar)) {
		return;
	}

	// Handle deferred image loading from OpenTileset
	if (pending_image_load_) {
		pending_image_load_ = false;
		if (!tileset_->source_image_path.empty()) {
			LoadSourceImage(engine, tileset_->source_image_path);
		}
	}

	RenderToolbar(engine);

	// Split panel: Grid on left, properties on right
	ImGui::BeginChild("GridArea", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 0), true);
	RenderTilesetGrid();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("PropertiesArea", ImVec2(0, 0), true);
	RenderTileProperties();
	ImGui::Separator();
	RenderTilePreview();
	ImGui::Separator();
	RenderTilePalette();
	ImGui::EndChild();

	EndPanel();
}

void TilesetEditorPanel::RenderToolbar(engine::Engine& engine) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewTileset();
			}
			if (ImGui::MenuItem("Open...")) {
				open_dialog_.Open();
			}
			if (ImGui::MenuItem("Save", nullptr, false, !current_file_path_.empty())) {
				SaveTileset(current_file_path_);
			}
			if (ImGui::MenuItem("Save As...")) {
				save_dialog_.Open("tileset.json");
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	open_dialog_.Render();
	save_dialog_.Render();
	image_dialog_.Render();

	// Source image selection
	ImGui::Text("Source Image:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 150);
	ImGui::InputText("##image_path", image_path_buffer_, sizeof(image_path_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Browse##img")) {
		image_dialog_.Open();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		LoadSourceImage(engine, image_path_buffer_);
	}

	if (!load_error_message_.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", load_error_message_.c_str());
	}

	if (loaded_image_) {
		ImGui::TextColored(
				ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
				"Loaded: %s (%dx%d)",
				tileset_->source_image_path.c_str(),
				loaded_image_->width,
				loaded_image_->height);
	}
	else {
		ImGui::TextDisabled("No image loaded (using placeholder grid)");
	}

	// Grid config controls (tile size, gap, padding)
	if (RenderGridConfigUI(tileset_->grid)) {
		SetDirty(true);
	}

	// Zoom control
	ImGui::SameLine();
	ImGui::Text("Zoom:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::SliderFloat("##zoom", &tile_display_scale_, 0.5f, 4.0f, "%.1fx");
}

void TilesetEditorPanel::LoadSourceImage(engine::Engine& engine, const std::string& path) {
	load_error_message_.clear();

	if (path.empty()) {
		load_error_message_ = "Please enter an image path.";
		return;
	}

	// TODO: Add LoadImage overload that takes a path and update this to use it
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), image_dialog_.RootDirectory());
	const auto image = engine::assets::AssetManager::LoadImage(relative_path.string());
	if (!image || !image->IsValid()) {
		load_error_message_ = "Failed to load image: " + path;
		return;
	}

	// Create GPU texture with nearest-neighbor filtering for pixel art
	engine::rendering::TextureParameters params;
	params.min_filter = engine::rendering::TextureFilter::Nearest;
	params.mag_filter = engine::rendering::TextureFilter::Nearest;
	params.wrap_s = engine::rendering::TextureWrap::ClampToEdge;
	params.wrap_t = engine::rendering::TextureWrap::ClampToEdge;

	auto& tex_mgr = engine.renderer->GetTextureManager();

	// Clean up previous texture
	if (gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		tex_mgr.DestroyTexture(gpu_texture_id_);
		gpu_texture_id_ = engine::rendering::INVALID_TEXTURE;
	}

	gpu_texture_id_ = tex_mgr.CreateTexture(image, params);
	if (gpu_texture_id_ == engine::rendering::INVALID_TEXTURE) {
		load_error_message_ = "Failed to create GPU texture from image.";
		return;
	}

	loaded_image_ = image;
	tileset_->source_image_path = path;
	std::strncpy(image_path_buffer_, path.c_str(), sizeof(image_path_buffer_) - 1);
	image_path_buffer_[sizeof(image_path_buffer_) - 1] = '\0';
	selected_tiles_.clear();
}

int TilesetEditorPanel::GetImageWidth() const { return loaded_image_ ? loaded_image_->width : PLACEHOLDER_IMAGE_WIDTH; }

int TilesetEditorPanel::GetImageHeight() const {
	return loaded_image_ ? loaded_image_->height : PLACEHOLDER_IMAGE_HEIGHT;
}

void TilesetEditorPanel::RenderTilesetGrid() {
	const int image_width = GetImageWidth();
	const int image_height = GetImageHeight();
	const auto& grid = tileset_->grid;

	// Calculate grid dimensions
	const int grid_cols = grid.GetColumns(image_width);
	const int grid_rows = grid.GetRows(image_height);

	if (grid_cols == 0 || grid_rows == 0) {
		ImGui::TextWrapped("Invalid tile size. Please set a valid tile width and height.");
		return;
	}

	if (loaded_image_) {
		ImGui::Text("Image: %dx%d | Grid: %d x %d tiles", image_width, image_height, grid_cols, grid_rows);
	}
	else {
		ImGui::Text("Placeholder Grid: %d x %d tiles", grid_cols, grid_rows);
	}

	// Calculate display sizes
	const float scale = tile_display_scale_;
	const float tile_display_w = grid.cell_width * scale;
	const float tile_display_h = grid.cell_height * scale;

	// Get draw list for custom rendering
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 grid_origin = ImGui::GetCursorScreenPos();
	const ImVec2 mouse_pos = ImGui::GetMousePos();

	// Reserve space for the grid (full image scaled)
	const float grid_width = static_cast<float>(image_width) * scale;
	const float grid_height = static_cast<float>(image_height) * scale;
	ImGui::InvisibleButton("TilesetCanvas", ImVec2(grid_width, grid_height));

	// Handle tile selection
	if (ImGui::IsItemHovered()) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			HandleTileSelection(mouse_pos, grid_origin);
			is_selecting_ = true;
		}
	}

	if (is_selecting_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		HandleTileSelection(mouse_pos, grid_origin);
	}
	else {
		is_selecting_ = false;
	}

	// Draw the tileset image or placeholder
	if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
		if (gl_tex != nullptr) {
			const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
			const ImVec2 img_min = grid_origin;
			const auto img_max = ImVec2(grid_origin.x + grid_width, grid_origin.y + grid_height);
			// UV flipped vertically for OpenGL
			draw_list->AddImage(imgui_tex, img_min, img_max, ImVec2(0, 1), ImVec2(1, 0));
		}
	}
	else {
		// Draw placeholder checkerboard
		for (int y = 0; y < grid_rows; ++y) {
			for (int x = 0; x < grid_cols; ++x) {
				const auto cell_off = grid.CellOriginScaled(x, y, scale);
				const auto tile_min = ImVec2(grid_origin.x + cell_off.x, grid_origin.y + cell_off.y);
				const auto tile_max = ImVec2(tile_min.x + tile_display_w, tile_min.y + tile_display_h);

				const bool is_dark = ((x + y) % 2) == 0;
				const ImU32 tile_color = is_dark ? IM_COL32(100, 100, 100, 255) : IM_COL32(120, 120, 120, 255);
				draw_list->AddRectFilled(tile_min, tile_max, tile_color);
			}
		}
	}

	// Draw grid overlay and selection highlights
	for (int y = 0; y < grid_rows; ++y) {
		for (int x = 0; x < grid_cols; ++x) {
			const uint32_t tile_id = GetTileIdFromCoords(x, y);
			const auto cell_off = grid.CellOriginScaled(x, y, scale);
			const auto tile_min = ImVec2(grid_origin.x + cell_off.x, grid_origin.y + cell_off.y);
			const auto tile_max = ImVec2(tile_min.x + tile_display_w, tile_min.y + tile_display_h);

			// Highlight selected tiles
			if (std::ranges::find(selected_tiles_, tile_id) != selected_tiles_.end()) {
				draw_list->AddRectFilled(tile_min, tile_max, IM_COL32(255, 255, 0, 60));
				draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
			}

			// Draw grid lines
			draw_list->AddRect(tile_min, tile_max, IM_COL32(60, 60, 60, 180));
		}
	}

	// Hover highlight
	if (ImGui::IsItemHovered()) {
		int hover_x = 0;
		int hover_y = 0;
		if (grid.PixelToCell(mouse_pos.x - grid_origin.x, mouse_pos.y - grid_origin.y, scale, hover_x, hover_y)
			&& hover_x >= 0 && hover_x < grid_cols && hover_y >= 0 && hover_y < grid_rows) {
			const auto cell_off = grid.CellOriginScaled(hover_x, hover_y, scale);
			const auto hover_min = ImVec2(grid_origin.x + cell_off.x, grid_origin.y + cell_off.y);
			const auto hover_max = ImVec2(hover_min.x + tile_display_w, hover_min.y + tile_display_h);
			draw_list->AddRect(hover_min, hover_max, IM_COL32(255, 255, 255, 180), 0.0f, 0, 2.0f);

			const uint32_t hover_id = GetTileIdFromCoords(hover_x, hover_y);
			ImGui::SetTooltip("Tile %u (%d, %d)", hover_id, hover_x, hover_y);
		}
	}
}

void TilesetEditorPanel::RenderTileProperties() {
	ImGui::Text("Tile Properties");
	ImGui::Separator();

	if (selected_tiles_.empty()) {
		ImGui::TextWrapped("No tile selected. Click a tile in the grid to select it.");
		return;
	}

	if (selected_tiles_.size() > 1) {
		ImGui::TextWrapped("Multiple tiles selected (%zu tiles)", selected_tiles_.size());
		ImGui::TextWrapped("Multi-tile editing not yet implemented.");
		return;
	}

	// Edit single tile properties
	const uint32_t selected_id = selected_tiles_[0];
	TileDefinition* tile_def = EnsureTileDefinition(selected_id);

	ImGui::Text("Tile ID: %u", selected_id);
	ImGui::Separator();

	// Tile name
	char name_buffer[256];
	std::strncpy(name_buffer, tile_def->name.c_str(), sizeof(name_buffer) - 1);
	name_buffer[sizeof(name_buffer) - 1] = '\0';
	if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
		tile_def->name = name_buffer;
		SetDirty(true);
	}

	// Collision flag
	if (ImGui::Checkbox("Collision", &tile_def->collision)) {
		SetDirty(true);
	}

	ImGui::Separator();
	ImGui::Text("Tags");

	// Display existing tags
	for (size_t i = 0; i < tile_def->tags.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		ImGui::BulletText("%s", tile_def->tags[i].c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			tile_def->tags.erase(tile_def->tags.begin() + i);
			SetDirty(true);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	// Add new tag
	ImGui::InputText("##new_tag", new_tag_buffer_, sizeof(new_tag_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Add Tag") && new_tag_buffer_[0] != '\0') {
		tile_def->tags.emplace_back(new_tag_buffer_);
		new_tag_buffer_[0] = '\0';
		SetDirty(true);
	}

	ImGui::Separator();
	ImGui::Text("Custom Properties");

	// Display existing properties
	std::vector<std::string> keys_to_remove;
	for (auto& [key, value] : tile_def->custom_properties) {
		ImGui::PushID(key.c_str());
		ImGui::BulletText("%s", key.c_str());

		char value_buffer[256];
		std::strncpy(value_buffer, value.c_str(), sizeof(value_buffer) - 1);
		value_buffer[sizeof(value_buffer) - 1] = '\0';

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		if (ImGui::InputText("##value", value_buffer, sizeof(value_buffer))) {
			value = value_buffer;
			SetDirty(true);
		}

		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			keys_to_remove.push_back(key);
			SetDirty(true);
		}
		ImGui::PopID();
	}

	// Remove marked properties
	for (const auto& key : keys_to_remove) {
		tile_def->custom_properties.erase(key);
	}

	// Add new property
	ImGui::InputText("Key", new_property_key_buffer_, sizeof(new_property_key_buffer_));
	ImGui::InputText("Value", new_property_value_buffer_, sizeof(new_property_value_buffer_));
	if (ImGui::Button("Add Property")) {
		if (new_property_key_buffer_[0] != '\0') {
			tile_def->custom_properties[new_property_key_buffer_] = new_property_value_buffer_;
			new_property_key_buffer_[0] = '\0';
			new_property_value_buffer_[0] = '\0';
			SetDirty(true);
		}
	}
}

void TilesetEditorPanel::RenderTilePreview() {
	ImGui::Text("Tile Preview");
	ImGui::Separator();

	if (selected_tiles_.empty()) {
		ImGui::TextDisabled("Select a tile to see preview.");
		return;
	}

	const uint32_t selected_id = selected_tiles_[0];
	int tile_x = 0;
	int tile_y = 0;
	GetCoordsFromTileId(selected_id, tile_x, tile_y);

	const int image_width = GetImageWidth();
	const int image_height = GetImageHeight();
	const int grid_cols = tileset_->grid.GetColumns(image_width);
	const int grid_rows = tileset_->grid.GetRows(image_height);

	if (tile_x >= grid_cols || tile_y >= grid_rows) {
		ImGui::TextDisabled("Tile out of range.");
		return;
	}

	constexpr float PREVIEW_SIZE = 96.0f;

	if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
		if (gl_tex != nullptr) {
			// Calculate UVs for this tile accounting for gap/padding (flipped vertically for OpenGL)
			const auto cell_px = tileset_->grid.CellOrigin(tile_x, tile_y);
			const float u0 = cell_px.x / static_cast<float>(image_width);
			const float u1 = (cell_px.x + tileset_->grid.cell_width) / static_cast<float>(image_width);
			const float v0 = 1.0f - cell_px.y / static_cast<float>(image_height);
			const float v1 = 1.0f - (cell_px.y + tileset_->grid.cell_height) / static_cast<float>(image_height);

			const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
			ImGui::Image(imgui_tex, ImVec2(PREVIEW_SIZE, PREVIEW_SIZE), ImVec2(u0, v0), ImVec2(u1, v1));
		}
	}
	else {
		// Placeholder preview
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 preview_origin = ImGui::GetCursorScreenPos();
		const auto preview_max = ImVec2(preview_origin.x + PREVIEW_SIZE, preview_origin.y + PREVIEW_SIZE);

		const bool is_dark = ((tile_x + tile_y) % 2) == 0;
		const ImU32 color = is_dark ? IM_COL32(100, 100, 100, 255) : IM_COL32(120, 120, 120, 255);
		draw_list->AddRectFilled(preview_origin, preview_max, color);
		draw_list->AddRect(preview_origin, preview_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);

		const std::string id_text = std::to_string(selected_id);
		const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
		const auto text_pos =
				ImVec2(preview_origin.x + (PREVIEW_SIZE - text_size.x) * 0.5f,
					   preview_origin.y + (PREVIEW_SIZE - text_size.y) * 0.5f);
		draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), id_text.c_str());

		ImGui::Dummy(ImVec2(PREVIEW_SIZE, PREVIEW_SIZE));
	}

	ImGui::Text("Tile %u  (%d, %d)", selected_id, tile_x, tile_y);
}

void TilesetEditorPanel::RenderTilePalette() {
	ImGui::Separator();
	ImGui::Text("Tile Palette & Brush");
	ImGui::Separator();

	// Brush mode selection
	ImGui::Text("Brush Mode:");
	if (ImGui::RadioButton("Single Tile", brush_mode_ == BrushMode::SingleTile)) {
		brush_mode_ = BrushMode::SingleTile;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Rectangle Fill", brush_mode_ == BrushMode::RectangleFill)) {
		brush_mode_ = BrushMode::RectangleFill;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Eraser", brush_mode_ == BrushMode::Eraser)) {
		brush_mode_ = BrushMode::Eraser;
	}

	ImGui::Separator();

	// Show selected tiles as palette
	if (!selected_tiles_.empty()) {
		ImGui::Text("Selected Tiles (%zu):", selected_tiles_.size());

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 palette_origin = ImGui::GetCursorScreenPos();

		constexpr float PALETTE_TILE_SIZE = 32.0f;
		const int image_width = GetImageWidth();
		const int grid_cols = tileset_->grid.GetColumns(image_width);

		for (size_t i = 0; i < selected_tiles_.size(); ++i) {
			const auto tile_min = ImVec2(palette_origin.x + i * (PALETTE_TILE_SIZE + 4), palette_origin.y);
			const auto tile_max = ImVec2(tile_min.x + PALETTE_TILE_SIZE, tile_min.y + PALETTE_TILE_SIZE);

			if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
				if (const auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_); gl_tex != nullptr) {
					int tx = 0;
					int ty = 0;
					GetCoordsFromTileId(selected_tiles_[i], tx, ty);
					const auto cell_px = tileset_->grid.CellOrigin(tx, ty);
					const float u0 = cell_px.x / static_cast<float>(image_width);
					const float u1 = (cell_px.x + tileset_->grid.cell_width) / static_cast<float>(image_width);
					const float v0 = 1.0f - cell_px.y / static_cast<float>(image_width);
					const float v1 = 1.0f - (cell_px.y + tileset_->grid.cell_height) / static_cast<float>(image_width);
					const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
					draw_list->AddImage(imgui_tex, tile_min, tile_max, ImVec2(u0, v0), ImVec2(u1, v1));
				}
			}
			else {
				draw_list->AddRectFilled(tile_min, tile_max, IM_COL32(100, 150, 200, 255));
				const std::string id_text = std::to_string(selected_tiles_[i]);
				const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
				const auto text_pos =
						ImVec2(tile_min.x + (PALETTE_TILE_SIZE - text_size.x) * 0.5f,
							   tile_min.y + (PALETTE_TILE_SIZE - text_size.y) * 0.5f);
				draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), id_text.c_str());
			}

			draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 255, 255));
		}

		// Reserve space for palette
		constexpr float palette_height = PALETTE_TILE_SIZE + 4;
		ImGui::Dummy(ImVec2(selected_tiles_.size() * (PALETTE_TILE_SIZE + 4), palette_height));
	}
	else {
		ImGui::TextWrapped("No tiles selected for brush.");
	}
}

void TilesetEditorPanel::NewTileset() {
	tileset_ = std::make_unique<TilesetDefinition>();
	selected_tiles_.clear();
	current_file_path_.clear();
	loaded_image_.reset();
	gpu_texture_id_ = engine::rendering::INVALID_TEXTURE;
	image_path_buffer_[0] = '\0';
	load_error_message_.clear();
	SetDirty(false);
}

bool TilesetEditorPanel::SaveTileset(const std::string& path) {
	try {
		json j;
		j["asset_type"] = "tileset";
		j["source_image_path"] = tileset_->source_image_path;
		j["tile_width"] = tileset_->grid.cell_width;
		j["tile_height"] = tileset_->grid.cell_height;
		j["gap_x"] = tileset_->grid.gap_x;
		j["gap_y"] = tileset_->grid.gap_y;
		j["padding_x"] = tileset_->grid.padding_x;
		j["padding_y"] = tileset_->grid.padding_y;

		json tiles_array = json::array();
		for (const auto& tile : tileset_->tiles) {
			json tile_json;
			tile_json["id"] = tile.id;
			tile_json["name"] = tile.name;
			tile_json["collision"] = tile.collision;
			tile_json["tags"] = tile.tags;
			tile_json["custom_properties"] = tile.custom_properties;
			tiles_array.push_back(tile_json);
		}
		j["tiles"] = tiles_array;

		if (!engine::assets::AssetManager::SaveTextFile(std::filesystem::path(path), j.dump(2))) {
			return false;
		}

		current_file_path_ = path;
		SetDirty(false);
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

bool TilesetEditorPanel::LoadTileset(const std::string& path) {
	try {
		auto text = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
		if (!text) {
			return false;
		}

		json j = json::parse(*text);

		auto new_tileset = std::make_unique<TilesetDefinition>();
		new_tileset->source_image_path = j.value("source_image_path", "");
		new_tileset->grid.cell_width = j.value("tile_width", 32);
		new_tileset->grid.cell_height = j.value("tile_height", 32);
		new_tileset->grid.gap_x = j.value("gap_x", 0);
		new_tileset->grid.gap_y = j.value("gap_y", 0);
		new_tileset->grid.padding_x = j.value("padding_x", 0);
		new_tileset->grid.padding_y = j.value("padding_y", 0);

		if (j.contains("tiles") && j["tiles"].is_array()) {
			for (const auto& tile_json : j["tiles"]) {
				TileDefinition tile;
				tile.id = tile_json.value("id", 0u);
				tile.name = tile_json.value("name", "");
				tile.collision = tile_json.value("collision", false);

				if (tile_json.contains("tags") && tile_json["tags"].is_array()) {
					for (const auto& tag : tile_json["tags"]) {
						tile.tags.push_back(tag.get<std::string>());
					}
				}

				if (tile_json.contains("custom_properties") && tile_json["custom_properties"].is_object()) {
					for (auto& [key, value] : tile_json["custom_properties"].items()) {
						tile.custom_properties[key] = value.get<std::string>();
					}
				}

				new_tileset->tiles.push_back(tile);
			}
		}

		tileset_ = std::move(new_tileset);
		current_file_path_ = path;
		selected_tiles_.clear();

		// Populate image path buffer so user can see the source image path
		std::strncpy(image_path_buffer_, tileset_->source_image_path.c_str(), sizeof(image_path_buffer_) - 1);
		image_path_buffer_[sizeof(image_path_buffer_) - 1] = '\0';

		SetDirty(false);
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

void TilesetEditorPanel::OpenTileset(const std::string& path) {
	if (LoadTileset(path)) {
		SetVisible(true);
		// Defer image loading until next Render() call which has engine access
		pending_image_load_ = !tileset_->source_image_path.empty();
	}
}

uint32_t TilesetEditorPanel::GetTileIdFromCoords(int x, int y) const {
	const int grid_cols = tileset_->grid.GetColumns(GetImageWidth());
	return static_cast<uint32_t>(y * grid_cols + x);
}

void TilesetEditorPanel::GetCoordsFromTileId(uint32_t id, int& out_x, int& out_y) const {
	const int grid_cols = tileset_->grid.GetColumns(GetImageWidth());
	out_x = (grid_cols > 0) ? static_cast<int>(id % grid_cols) : 0;
	out_y = (grid_cols > 0) ? static_cast<int>(id / grid_cols) : 0;
}

void TilesetEditorPanel::HandleTileSelection(const ImVec2& mouse_pos, const ImVec2& grid_origin) {
	const float scale = tile_display_scale_;
	const auto& grid = tileset_->grid;

	int x = 0;
	int y = 0;
	if (!grid.PixelToCell(mouse_pos.x - grid_origin.x, mouse_pos.y - grid_origin.y, scale, x, y)) {
		return;
	}

	const int image_width = GetImageWidth();
	const int image_height = GetImageHeight();
	const int grid_cols = grid.GetColumns(image_width);
	const int grid_rows = grid.GetRows(image_height);

	// Check bounds
	if (x < 0 || x >= grid_cols || y < 0 || y >= grid_rows) {
		return;
	}

	const uint32_t clicked_id = GetTileIdFromCoords(x, y);

	// Handle shift-click for multi-select
	if (ImGui::GetIO().KeyShift && !selected_tiles_.empty()) {
		// Add range from last selected to current
		const uint32_t last_id = selected_tiles_.back();
		const uint32_t min_id = std::min(last_id, clicked_id);
		const uint32_t max_id = std::max(last_id, clicked_id);

		selected_tiles_.clear();
		for (uint32_t id = min_id; id <= max_id; ++id) {
			selected_tiles_.push_back(id);
		}
	}
	else {
		// Single selection
		selected_tiles_.clear();
		selected_tiles_.push_back(clicked_id);
	}
}

TileDefinition* TilesetEditorPanel::EnsureTileDefinition(uint32_t id) {
	// Check if tile already exists
	if (TileDefinition* existing = tileset_->GetTile(id); existing != nullptr) {
		return existing;
	}

	// Create new tile definition
	tileset_->tiles.emplace_back(id);
	return &tileset_->tiles.back();
}

} // namespace editor
