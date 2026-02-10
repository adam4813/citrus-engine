#include "tileset_editor_panel.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

using json = nlohmann::json;

namespace editor {

TilesetEditorPanel::TilesetEditorPanel() : tileset_(std::make_unique<TilesetDefinition>()) {}

TilesetEditorPanel::~TilesetEditorPanel() = default;

void TilesetEditorPanel::Render() {
	if (!is_visible_) {
		return;
	}

	ImGui::Begin("Tileset Editor", &is_visible_, ImGuiWindowFlags_MenuBar);

	RenderToolbar();

	// Split panel: Grid on left, properties on right
	ImGui::BeginChild("GridArea", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 0), true);
	RenderTilesetGrid();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("PropertiesArea", ImVec2(0, 0), true);
	RenderTileProperties();
	ImGui::Separator();
	RenderTilePalette();
	ImGui::EndChild();

	ImGui::End();
}

void TilesetEditorPanel::RenderToolbar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewTileset();
			}
			if (ImGui::MenuItem("Open...")) {
				// Placeholder: In real implementation, open file dialog
				// For now, use a hardcoded path
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
				// Placeholder: In real implementation, open save file dialog
				SaveTileset("tileset.json");
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Toolbar buttons
	ImGui::Text("Grid Size:");
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

	ImGui::SameLine();
	ImGui::Separator();

	// Source image path
	ImGui::SameLine();
	ImGui::Text("Source Image: %s", tileset_->source_image_path.empty() 
		? "(none)" 
		: tileset_->source_image_path.c_str());
}

void TilesetEditorPanel::RenderTilesetGrid() {
	ImGui::Text("Tileset Grid (Placeholder)");
	ImGui::Separator();

	// Calculate grid dimensions
	const int grid_cols = tileset_->GetGridColumns(PLACEHOLDER_IMAGE_WIDTH);
	const int grid_rows = tileset_->GetGridRows(PLACEHOLDER_IMAGE_HEIGHT);

	if (grid_cols == 0 || grid_rows == 0) {
		ImGui::TextWrapped("Invalid tile size. Please set a valid tile width and height.");
		return;
	}

	ImGui::Text("Grid: %d x %d tiles", grid_cols, grid_rows);

	// Get draw list for custom rendering
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 grid_origin = ImGui::GetCursorScreenPos();
	const ImVec2 mouse_pos = ImGui::GetMousePos();

	// Reserve space for the grid
	const float grid_width = grid_cols * TILE_DISPLAY_SIZE;
	const float grid_height = grid_rows * TILE_DISPLAY_SIZE;
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

	// Draw grid and tiles
	for (int y = 0; y < grid_rows; ++y) {
		for (int x = 0; x < grid_cols; ++x) {
			const uint32_t tile_id = GetTileIdFromCoords(x, y);
			const ImVec2 tile_min = ImVec2(
				grid_origin.x + x * TILE_DISPLAY_SIZE,
				grid_origin.y + y * TILE_DISPLAY_SIZE
			);
			const ImVec2 tile_max = ImVec2(
				tile_min.x + TILE_DISPLAY_SIZE,
				tile_min.y + TILE_DISPLAY_SIZE
			);

			// Draw placeholder tile (checkerboard pattern)
			const bool is_dark = ((x + y) % 2) == 0;
			const ImU32 tile_color = is_dark 
				? IM_COL32(100, 100, 100, 255) 
				: IM_COL32(120, 120, 120, 255);
			draw_list->AddRectFilled(tile_min, tile_max, tile_color);

			// Highlight selected tiles
			const bool is_selected = std::ranges::find(selected_tiles_, tile_id) != selected_tiles_.end();
			if (is_selected) {
				draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
			}

			// Draw grid lines
			draw_list->AddRect(tile_min, tile_max, IM_COL32(60, 60, 60, 255));

			// Draw tile ID
			const std::string id_text = std::to_string(tile_id);
			const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
			const ImVec2 text_pos = ImVec2(
				tile_min.x + (TILE_DISPLAY_SIZE - text_size.x) * 0.5f,
				tile_min.y + (TILE_DISPLAY_SIZE - text_size.y) * 0.5f
			);
			draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 200), id_text.c_str());
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
		
		// Draw selected tiles in a horizontal strip
		constexpr float PALETTE_TILE_SIZE = 32.0f;
		for (size_t i = 0; i < selected_tiles_.size(); ++i) {
			const ImVec2 tile_min = ImVec2(
				palette_origin.x + i * (PALETTE_TILE_SIZE + 4),
				palette_origin.y
			);
			const ImVec2 tile_max = ImVec2(
				tile_min.x + PALETTE_TILE_SIZE,
				tile_min.y + PALETTE_TILE_SIZE
			);

			// Draw placeholder
			draw_list->AddRectFilled(tile_min, tile_max, IM_COL32(100, 150, 200, 255));
			draw_list->AddRect(tile_min, tile_max, IM_COL32(255, 255, 255, 255));

			// Draw tile ID
			const std::string id_text = std::to_string(selected_tiles_[i]);
			const ImVec2 text_size = ImGui::CalcTextSize(id_text.c_str());
			const ImVec2 text_pos = ImVec2(
				tile_min.x + (PALETTE_TILE_SIZE - text_size.x) * 0.5f,
				tile_min.y + (PALETTE_TILE_SIZE - text_size.y) * 0.5f
			);
			draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), id_text.c_str());
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

		std::ofstream file(path);
		if (!file.is_open()) {
			return false;
		}

		file << j.dump(2); // Pretty print with 2-space indentation
		current_file_path_ = path;
		return true;
	} catch (const std::exception&) {
		return false;
	}
}

bool TilesetEditorPanel::LoadTileset(const std::string& path) {
	try {
		std::ifstream file(path);
		if (!file.is_open()) {
			return false;
		}

		json j;
		file >> j;

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
	const int grid_cols = tileset_->GetGridColumns(PLACEHOLDER_IMAGE_WIDTH);
	return static_cast<uint32_t>(y * grid_cols + x);
}

void TilesetEditorPanel::GetCoordsFromTileId(uint32_t id, int& out_x, int& out_y) const {
	const int grid_cols = tileset_->GetGridColumns(PLACEHOLDER_IMAGE_WIDTH);
	out_x = static_cast<int>(id % grid_cols);
	out_y = static_cast<int>(id / grid_cols);
}

void TilesetEditorPanel::HandleTileSelection(const ImVec2& mouse_pos, const ImVec2& grid_origin) {
	const ImVec2 relative_pos = ImVec2(mouse_pos.x - grid_origin.x, mouse_pos.y - grid_origin.y);
	
	const int x = static_cast<int>(relative_pos.x / TILE_DISPLAY_SIZE);
	const int y = static_cast<int>(relative_pos.y / TILE_DISPLAY_SIZE);

	const int grid_cols = tileset_->GetGridColumns(PLACEHOLDER_IMAGE_WIDTH);
	const int grid_rows = tileset_->GetGridRows(PLACEHOLDER_IMAGE_HEIGHT);

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
