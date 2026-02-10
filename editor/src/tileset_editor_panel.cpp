#include "tileset_editor_panel.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>

using json = nlohmann::json;

namespace editor {

TilesetEditorPanel::TilesetEditorPanel() : tileset_(std::make_unique<TilesetDefinition>()) {}

TilesetEditorPanel::~TilesetEditorPanel() = default;

void TilesetEditorPanel::Render(engine::Engine& engine) {
	if (!is_visible_) {
		return;
	}

	ImGui::Begin("Tileset Editor", &is_visible_, ImGuiWindowFlags_MenuBar);

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

	ImGui::End();
}

void TilesetEditorPanel::RenderToolbar(engine::Engine& engine) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewTileset();
			}
			if (ImGui::MenuItem("Open...")) {
				LoadTileset("tileset.json");
			}
			if (ImGui::MenuItem("Save")) {
				if (!current_file_path_.empty()) {
					SaveTileset(current_file_path_);
				} else {
					SaveTileset("tileset.json");
				}
			}
			if (ImGui::MenuItem("Save As...")) {
				SaveTileset("tileset.json");
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Source image selection
	ImGui::Text("Source Image:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
	ImGui::InputText("##image_path", image_path_buffer_, sizeof(image_path_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		LoadSourceImage(engine, image_path_buffer_);
	}

	if (!load_error_message_.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", load_error_message_.c_str());
	}

	if (loaded_image_) {
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded: %s (%dx%d)",
			tileset_->source_image_path.c_str(), loaded_image_->width, loaded_image_->height);
	} else {
		ImGui::TextDisabled("No image loaded (using placeholder grid)");
	}

	// Tile size controls
	ImGui::Text("Tile Size:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##tile_width", &tileset_->tile_width, 1, 10);
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##tile_height", &tileset_->tile_height, 1, 10);

	// Clamp to reasonable values
	tileset_->tile_width = std::max(1, std::min(512, tileset_->tile_width));
	tileset_->tile_height = std::max(1, std::min(512, tileset_->tile_height));

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

	auto image = engine::assets::AssetManager::LoadImage(path);
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

int TilesetEditorPanel::GetImageWidth() const {
	return loaded_image_ ? loaded_image_->width : PLACEHOLDER_IMAGE_WIDTH;
}

int TilesetEditorPanel::GetImageHeight() const {
	return loaded_image_ ? loaded_image_->height : PLACEHOLDER_IMAGE_HEIGHT;
}

void TilesetEditorPanel::RenderTilesetGrid() {
	const int image_width = GetImageWidth();
	const int image_height = GetImageHeight();

	// Calculate grid dimensions
	const int grid_cols = tileset_->GetGridColumns(image_width);
	const int grid_rows = tileset_->GetGridRows(image_height);

	if (grid_cols == 0 || grid_rows == 0) {
		ImGui::TextWrapped("Invalid tile size. Please set a valid tile width and height.");
		return;
	}

	if (loaded_image_) {
		ImGui::Text("Image: %dx%d | Grid: %d x %d tiles", image_width, image_height, grid_cols, grid_rows);
	} else {
		ImGui::Text("Placeholder Grid: %d x %d tiles", grid_cols, grid_rows);
	}

	// Calculate display sizes
	const float tile_display_w = tileset_->tile_width * tile_display_scale_;
	const float tile_display_h = tileset_->tile_height * tile_display_scale_;

	// Get draw list for custom rendering
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 grid_origin = ImGui::GetCursorScreenPos();
	const ImVec2 mouse_pos = ImGui::GetMousePos();

	// Reserve space for the grid
	const float grid_width = grid_cols * tile_display_w;
	const float grid_height = grid_rows * tile_display_h;
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
	} else {
		is_selecting_ = false;
	}

	// Draw the tileset image or placeholder
	if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
		if (gl_tex != nullptr) {
			const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
			const ImVec2 img_min = grid_origin;
			const ImVec2 img_max = ImVec2(grid_origin.x + grid_width, grid_origin.y + grid_height);
			// UV flipped vertically for OpenGL
			draw_list->AddImage(imgui_tex, img_min, img_max, ImVec2(0, 1), ImVec2(1, 0));
		}
	} else {
		// Draw placeholder checkerboard
		for (int y = 0; y < grid_rows; ++y) {
			for (int x = 0; x < grid_cols; ++x) {
				const ImVec2 tile_min = ImVec2(
					grid_origin.x + x * tile_display_w,
					grid_origin.y + y * tile_display_h);
				const ImVec2 tile_max = ImVec2(
					tile_min.x + tile_display_w,
					tile_min.y + tile_display_h);

				const bool is_dark = ((x + y) % 2) == 0;
				const ImU32 tile_color = is_dark
					? IM_COL32(100, 100, 100, 255)
					: IM_COL32(120, 120, 120, 255);
				draw_list->AddRectFilled(tile_min, tile_max, tile_color);
			}
		}
	}

	// Draw grid overlay and selection highlights
	for (int y = 0; y < grid_rows; ++y) {
		for (int x = 0; x < grid_cols; ++x) {
			const uint32_t tile_id = GetTileIdFromCoords(x, y);
			const ImVec2 tile_min = ImVec2(
				grid_origin.x + x * tile_display_w,
				grid_origin.y + y * tile_display_h);
			const ImVec2 tile_max = ImVec2(
				tile_min.x + tile_display_w,
				tile_min.y + tile_display_h);

			// Highlight selected tiles
			const bool is_selected = std::ranges::find(selected_tiles_, tile_id) != selected_tiles_.end();
			if (is_selected) {
				draw_list->AddRectFilled(tile_min, tile_max, IM_COL32(255, 255, 0, 60));
				draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
			}

			// Draw grid lines
			draw_list->AddRect(tile_min, tile_max, IM_COL32(60, 60, 60, 180));
		}
	}

	// Hover highlight
	if (ImGui::IsItemHovered()) {
		const int hover_x = static_cast<int>((mouse_pos.x - grid_origin.x) / tile_display_w);
		const int hover_y = static_cast<int>((mouse_pos.y - grid_origin.y) / tile_display_h);
		if (hover_x >= 0 && hover_x < grid_cols && hover_y >= 0 && hover_y < grid_rows) {
			const ImVec2 hover_min = ImVec2(
				grid_origin.x + hover_x * tile_display_w,
				grid_origin.y + hover_y * tile_display_h);
			const ImVec2 hover_max = ImVec2(
				hover_min.x + tile_display_w,
				hover_min.y + tile_display_h);
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
	}

	// Collision flag
	ImGui::Checkbox("Collision", &tile_def->collision);

	ImGui::Separator();
	ImGui::Text("Tags");

	// Display existing tags
	for (size_t i = 0; i < tile_def->tags.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		ImGui::BulletText("%s", tile_def->tags[i].c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			tile_def->tags.erase(tile_def->tags.begin() + i);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	// Add new tag
	ImGui::InputText("##new_tag", new_tag_buffer_, sizeof(new_tag_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Add Tag")) {
		if (new_tag_buffer_[0] != '\0') {
			tile_def->tags.emplace_back(new_tag_buffer_);
			new_tag_buffer_[0] = '\0';
		}
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
		}
		
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			keys_to_remove.push_back(key);
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
	const int grid_cols = tileset_->GetGridColumns(image_width);
	const int grid_rows = tileset_->GetGridRows(image_height);

	if (tile_x >= grid_cols || tile_y >= grid_rows) {
		ImGui::TextDisabled("Tile out of range.");
		return;
	}

	constexpr float PREVIEW_SIZE = 96.0f;

	if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
		if (gl_tex != nullptr) {
			// Calculate UVs for this tile (flipped vertically for OpenGL)
			const float u0 = static_cast<float>(tile_x * tileset_->tile_width) / static_cast<float>(image_width);
			const float u1 = static_cast<float>((tile_x + 1) * tileset_->tile_width) / static_cast<float>(image_width);
			const float v0 = 1.0f - static_cast<float>(tile_y * tileset_->tile_height) / static_cast<float>(image_height);
			const float v1 = 1.0f - static_cast<float>((tile_y + 1) * tileset_->tile_height) / static_cast<float>(image_height);

			const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
			ImGui::Image(imgui_tex, ImVec2(PREVIEW_SIZE, PREVIEW_SIZE), ImVec2(u0, v0), ImVec2(u1, v1));
		}
	} else {
		// Placeholder preview
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 preview_origin = ImGui::GetCursorScreenPos();
		const ImVec2 preview_max = ImVec2(preview_origin.x + PREVIEW_SIZE, preview_origin.y + PREVIEW_SIZE);

		const bool is_dark = ((tile_x + tile_y) % 2) == 0;
		const ImU32 color = is_dark ? IM_COL32(100, 100, 100, 255) : IM_COL32(120, 120, 120, 255);
		draw_list->AddRectFilled(preview_origin, preview_max, color);
		draw_list->AddRect(preview_origin, preview_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);

		const std::string id_text = std::to_string(selected_id);
		const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
		const ImVec2 text_pos = ImVec2(
			preview_origin.x + (PREVIEW_SIZE - text_size.x) * 0.5f,
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
		const int image_height = GetImageHeight();
		const int grid_cols = tileset_->GetGridColumns(image_width);

		for (size_t i = 0; i < selected_tiles_.size(); ++i) {
			const ImVec2 tile_min = ImVec2(
				palette_origin.x + i * (PALETTE_TILE_SIZE + 4),
				palette_origin.y);
			const ImVec2 tile_max = ImVec2(
				tile_min.x + PALETTE_TILE_SIZE,
				tile_min.y + PALETTE_TILE_SIZE);

			if (loaded_image_ && gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
				auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
				if (gl_tex != nullptr) {
					int tx = 0;
					int ty = 0;
					GetCoordsFromTileId(selected_tiles_[i], tx, ty);
					const float u0 = static_cast<float>(tx * tileset_->tile_width) / static_cast<float>(image_width);
					const float u1 = static_cast<float>((tx + 1) * tileset_->tile_width) / static_cast<float>(image_width);
					const float v0 = 1.0f - static_cast<float>(ty * tileset_->tile_height) / static_cast<float>(image_height);
					const float v1 = 1.0f - static_cast<float>((ty + 1) * tileset_->tile_height) / static_cast<float>(image_height);
					const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
					draw_list->AddImage(imgui_tex, tile_min, tile_max, ImVec2(u0, v0), ImVec2(u1, v1));
				}
			} else {
				draw_list->AddRectFilled(tile_min, tile_max, IM_COL32(100, 150, 200, 255));
				const std::string id_text = std::to_string(selected_tiles_[i]);
				const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
				const ImVec2 text_pos = ImVec2(
					tile_min.x + (PALETTE_TILE_SIZE - text_size.x) * 0.5f,
					tile_min.y + (PALETTE_TILE_SIZE - text_size.y) * 0.5f);
				draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), id_text.c_str());
			}

			draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 255, 255));
		}

		// Reserve space for palette
		const float palette_height = PALETTE_TILE_SIZE + 4;
		ImGui::Dummy(ImVec2(selected_tiles_.size() * (PALETTE_TILE_SIZE + 4), palette_height));
	} else {
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
}

bool TilesetEditorPanel::SaveTileset(const std::string& path) {
	try {
		json j;
		j["source_image_path"] = tileset_->source_image_path;
		j["tile_width"] = tileset_->tile_width;
		j["tile_height"] = tileset_->tile_height;

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
		return true;
	} catch (const std::exception&) {
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
		new_tileset->tile_width = j.value("tile_width", 32);
		new_tileset->tile_height = j.value("tile_height", 32);

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
		return true;
	} catch (const std::exception&) {
		return false;
	}
}

void TilesetEditorPanel::OpenTileset(const std::string& path) {
	if (LoadTileset(path)) {
		SetVisible(true);
	}
}

uint32_t TilesetEditorPanel::GetTileIdFromCoords(int x, int y) const {
	const int grid_cols = tileset_->GetGridColumns(GetImageWidth());
	return static_cast<uint32_t>(y * grid_cols + x);
}

void TilesetEditorPanel::GetCoordsFromTileId(uint32_t id, int& out_x, int& out_y) const {
	const int grid_cols = tileset_->GetGridColumns(GetImageWidth());
	out_x = (grid_cols > 0) ? static_cast<int>(id % grid_cols) : 0;
	out_y = (grid_cols > 0) ? static_cast<int>(id / grid_cols) : 0;
}

void TilesetEditorPanel::HandleTileSelection(const ImVec2& mouse_pos, const ImVec2& grid_origin) {
	const float tile_display_w = tileset_->tile_width * tile_display_scale_;
	const float tile_display_h = tileset_->tile_height * tile_display_scale_;

	const ImVec2 relative_pos = ImVec2(mouse_pos.x - grid_origin.x, mouse_pos.y - grid_origin.y);
	
	const int x = static_cast<int>(relative_pos.x / tile_display_w);
	const int y = static_cast<int>(relative_pos.y / tile_display_h);

	const int image_width = GetImageWidth();
	const int image_height = GetImageHeight();
	const int grid_cols = tileset_->GetGridColumns(image_width);
	const int grid_rows = tileset_->GetGridRows(image_height);

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
	} else {
		// Single selection
		selected_tiles_.clear();
		selected_tiles_.push_back(clicked_id);
	}
}

TileDefinition* TilesetEditorPanel::EnsureTileDefinition(uint32_t id) {
	// Check if tile already exists
	TileDefinition* existing = tileset_->GetTile(id);
	if (existing != nullptr) {
		return existing;
	}

	// Create new tile definition
	tileset_->tiles.emplace_back(id);
	return &tileset_->tiles.back();
}

} // namespace editor
