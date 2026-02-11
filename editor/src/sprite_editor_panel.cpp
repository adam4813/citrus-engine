#include "sprite_editor_panel.h"

#include "asset_editor_registry.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

using json = nlohmann::json;

namespace editor {

SpriteEditorPanel::SpriteEditorPanel() = default;

SpriteEditorPanel::~SpriteEditorPanel() = default;

std::string_view SpriteEditorPanel::GetPanelName() const { return "Sprite Editor"; }

void SpriteEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("sprite_atlas", [this](const std::string& path) { OpenAtlas(path); });
}

void SpriteEditorPanel::Render(engine::Engine& engine) {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Sprite Editor", &VisibleRef(), ImGuiWindowFlags_MenuBar);

	// Handle deferred image loading from OpenAtlas
	if (pending_image_load_) {
		pending_image_load_ = false;
		if (!pending_image_path_.empty()) {
			LoadSourceImage(engine, pending_image_path_);
			pending_image_path_.clear();
		}
	}

	RenderToolbar(engine);

	// Layout: canvas on left, sprite list + preview on right
	const float right_panel_width = 250.0f;
	const float available_width = ImGui::GetContentRegionAvail().x;
	const float canvas_width = available_width - right_panel_width - 8.0f;

	if (canvas_width > 100.0f) {
		ImGui::BeginChild("CanvasArea", ImVec2(canvas_width, 0), true);
		RenderCanvas();
		ImGui::EndChild();

		ImGui::SameLine();
	}

	ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
	RenderSpriteList();
	ImGui::Separator();
	RenderPreview();
	ImGui::EndChild();

	ImGui::End();
}

void SpriteEditorPanel::RenderToolbar(engine::Engine& engine) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open...")) {
				show_open_dialog_ = true;
			}
			if (ImGui::MenuItem("Export Atlas...")) {
				ExportAtlas();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// Open file dialog popup
	if (show_open_dialog_) {
		ImGui::OpenPopup("Open Sprite Atlas");
		show_open_dialog_ = false;
	}
	if (ImGui::BeginPopupModal("Open Sprite Atlas", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter sprite atlas file path:");
		ImGui::SetNextItemWidth(300);
		ImGui::InputText("##open_path", open_path_buffer_, sizeof(open_path_buffer_));
		ImGui::Separator();
		if (ImGui::Button("Open", ImVec2(120, 0))) {
			OpenAtlas(open_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Image path input
	ImGui::Text("Image:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
	ImGui::InputText("##image_path", image_path_buffer_, sizeof(image_path_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		LoadSourceImage(engine, image_path_buffer_);
	}

	// Status message
	if (!status_message_.empty()) {
		if (status_is_error_) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", status_message_.c_str());
		} else {
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", status_message_.c_str());
		}
	}

	// Grid controls
	ImGui::Text("Grid:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	ImGui::InputInt("##grid_w", &grid_width_, 0, 0);
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	ImGui::InputInt("##grid_h", &grid_height_, 0, 0);
	ImGui::SameLine();
	ImGui::Checkbox("Show Grid", &show_grid_);

	grid_width_ = std::max(1, std::min(512, grid_width_));
	grid_height_ = std::max(1, std::min(512, grid_height_));

	ImGui::SameLine();
	if (ImGui::Button("Auto Grid")) {
		AutoGrid();
	}

	ImGui::SameLine();
	ImGui::Text("Zoom:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::SliderFloat("##zoom", &canvas_scale_, 0.5f, 4.0f, "%.1fx");

	// Export controls
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	ImGui::InputText("##export_path", export_path_buffer_, sizeof(export_path_buffer_));
	ImGui::SameLine();
	if (ImGui::Button("Export")) {
		ExportAtlas();
	}
}

void SpriteEditorPanel::LoadSourceImage(engine::Engine& engine, const std::string& path) {
	status_message_.clear();
	status_is_error_ = false;

	if (path.empty()) {
		status_message_ = "Please enter an image path.";
		status_is_error_ = true;
		return;
	}

	auto image = engine::assets::AssetManager::LoadImage(path);
	if (!image || !image->IsValid()) {
		status_message_ = "Failed to load image: " + path;
		status_is_error_ = true;
		return;
	}

	// Create GPU texture with nearest-neighbor filtering for pixel art
	engine::rendering::TextureParameters params;
	params.min_filter = engine::rendering::TextureFilter::Nearest;
	params.mag_filter = engine::rendering::TextureFilter::Nearest;
	params.wrap_s = engine::rendering::TextureWrap::ClampToEdge;
	params.wrap_t = engine::rendering::TextureWrap::ClampToEdge;

	auto& tex_mgr = engine.renderer->GetTextureManager();

	if (gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		tex_mgr.DestroyTexture(gpu_texture_id_);
		gpu_texture_id_ = engine::rendering::INVALID_TEXTURE;
	}

	gpu_texture_id_ = tex_mgr.CreateTexture(image, params);
	if (gpu_texture_id_ == engine::rendering::INVALID_TEXTURE) {
		status_message_ = "Failed to create GPU texture.";
		status_is_error_ = true;
		return;
	}

	loaded_image_ = image;
	image_path_ = path;
	std::strncpy(image_path_buffer_, path.c_str(), sizeof(image_path_buffer_) - 1);
	image_path_buffer_[sizeof(image_path_buffer_) - 1] = '\0';

	status_message_ = "Loaded: " + path + " (" + std::to_string(image->width) + "x" + std::to_string(image->height) + ")";
}

void SpriteEditorPanel::RenderCanvas() {
	if (!loaded_image_) {
		ImGui::TextWrapped("Load an image to begin editing sprites.");
		return;
	}

	const int img_w = loaded_image_->width;
	const int img_h = loaded_image_->height;
	const float display_w = img_w * canvas_scale_;
	const float display_h = img_h * canvas_scale_;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 canvas_origin = ImGui::GetCursorScreenPos();
	const ImVec2 mouse_pos = ImGui::GetMousePos();

	// Reserve space
	ImGui::InvisibleButton("SpriteCanvas", ImVec2(display_w, display_h));
	const bool is_hovered = ImGui::IsItemHovered();

	// Draw the image
	if (gpu_texture_id_ != engine::rendering::INVALID_TEXTURE) {
		auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
		if (gl_tex != nullptr) {
			const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));
			const ImVec2 img_min = canvas_origin;
			const ImVec2 img_max = ImVec2(canvas_origin.x + display_w, canvas_origin.y + display_h);
			draw_list->AddImage(imgui_tex, img_min, img_max, ImVec2(0, 1), ImVec2(1, 0));
		}
	}

	// Draw grid overlay
	if (show_grid_ && grid_width_ > 0 && grid_height_ > 0) {
		const float cell_w = grid_width_ * canvas_scale_;
		const float cell_h = grid_height_ * canvas_scale_;
		const ImU32 grid_color = IM_COL32(255, 255, 255, 40);

		for (float gx = 0; gx <= display_w; gx += cell_w) {
			draw_list->AddLine(
				ImVec2(canvas_origin.x + gx, canvas_origin.y),
				ImVec2(canvas_origin.x + gx, canvas_origin.y + display_h),
				grid_color);
		}
		for (float gy = 0; gy <= display_h; gy += cell_h) {
			draw_list->AddLine(
				ImVec2(canvas_origin.x, canvas_origin.y + gy),
				ImVec2(canvas_origin.x + display_w, canvas_origin.y + gy),
				grid_color);
		}
	}

	// Draw existing sprite regions
	for (int i = 0; i < static_cast<int>(sprites_.size()); ++i) {
		const auto& sprite = sprites_[i];
		const ImVec2 rect_min = ImVec2(
			canvas_origin.x + sprite.x * canvas_scale_,
			canvas_origin.y + sprite.y * canvas_scale_);
		const ImVec2 rect_max = ImVec2(
			rect_min.x + sprite.width * canvas_scale_,
			rect_min.y + sprite.height * canvas_scale_);

		const bool is_selected = (i == selected_sprite_);
		const ImU32 fill_color = is_selected ? IM_COL32(255, 255, 0, 40) : IM_COL32(0, 200, 255, 30);
		const ImU32 border_color = is_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 200, 255, 200);
		const float thickness = is_selected ? 2.0f : 1.0f;

		draw_list->AddRectFilled(rect_min, rect_max, fill_color);
		draw_list->AddRect(rect_min, rect_max, border_color, 0.0f, 0, thickness);

		// Draw sprite name label
		if (!sprite.name.empty()) {
			draw_list->AddText(ImVec2(rect_min.x + 2, rect_min.y + 1), IM_COL32(255, 255, 255, 220), sprite.name.c_str());
		}
	}

	// Handle manual rectangle selection
	if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		is_selecting_ = true;
		selection_start_ = mouse_pos;
		selection_end_ = mouse_pos;
	}

	if (is_selecting_) {
		selection_end_ = mouse_pos;

		// Draw selection rectangle
		const ImU32 sel_fill = IM_COL32(255, 200, 0, 40);
		const ImU32 sel_border = IM_COL32(255, 200, 0, 200);
		draw_list->AddRectFilled(selection_start_, selection_end_, sel_fill);
		draw_list->AddRect(selection_start_, selection_end_, sel_border, 0.0f, 0, 1.5f);

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			is_selecting_ = false;

			// Convert screen coords to pixel coords
			const float min_x = std::min(selection_start_.x, selection_end_.x);
			const float min_y = std::min(selection_start_.y, selection_end_.y);
			const float max_x = std::max(selection_start_.x, selection_end_.x);
			const float max_y = std::max(selection_start_.y, selection_end_.y);

			int px = static_cast<int>((min_x - canvas_origin.x) / canvas_scale_);
			int py = static_cast<int>((min_y - canvas_origin.y) / canvas_scale_);
			int pw = static_cast<int>((max_x - min_x) / canvas_scale_);
			int ph = static_cast<int>((max_y - min_y) / canvas_scale_);

			// Clamp to image bounds
			px = std::max(0, std::min(px, img_w));
			py = std::max(0, std::min(py, img_h));
			pw = std::min(pw, img_w - px);
			ph = std::min(ph, img_h - py);

			// Only create region if it has meaningful size
			if (pw >= 2 && ph >= 2) {
				SpriteRegion region;
				region.name = "sprite_" + std::to_string(sprites_.size());
				region.x = px;
				region.y = py;
				region.width = pw;
				region.height = ph;
				sprites_.push_back(region);
				selected_sprite_ = static_cast<int>(sprites_.size()) - 1;
			}
		}
	}
}

void SpriteEditorPanel::RenderSpriteList() {
	ImGui::Text("Sprites (%zu)", sprites_.size());
	ImGui::Separator();

	if (sprites_.empty()) {
		ImGui::TextWrapped("No sprites defined. Use Auto Grid or click-drag on the canvas.");
		return;
	}

	for (int i = 0; i < static_cast<int>(sprites_.size()); ++i) {
		auto& sprite = sprites_[i];
		ImGui::PushID(i);

		const bool is_selected = (i == selected_sprite_);
		if (ImGui::Selectable(("##sprite_" + std::to_string(i)).c_str(), is_selected, 0, ImVec2(0, 20))) {
			selected_sprite_ = i;
		}

		ImGui::SameLine();

		// Editable name
		char name_buf[256];
		std::strncpy(name_buf, sprite.name.c_str(), sizeof(name_buf) - 1);
		name_buf[sizeof(name_buf) - 1] = '\0';
		ImGui::SetNextItemWidth(120);
		if (ImGui::InputText("##name", name_buf, sizeof(name_buf))) {
			sprite.name = name_buf;
		}

		ImGui::SameLine();
		ImGui::TextDisabled("(%d,%d %dx%d)", sprite.x, sprite.y, sprite.width, sprite.height);

		ImGui::SameLine();
		if (ImGui::SmallButton("X")) {
			sprites_.erase(sprites_.begin() + i);
			if (selected_sprite_ >= static_cast<int>(sprites_.size())) {
				selected_sprite_ = static_cast<int>(sprites_.size()) - 1;
			}
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}

	if (ImGui::Button("Clear All")) {
		sprites_.clear();
		selected_sprite_ = -1;
	}
}

void SpriteEditorPanel::RenderPreview() {
	ImGui::Text("Preview");
	ImGui::Separator();

	if (selected_sprite_ < 0 || selected_sprite_ >= static_cast<int>(sprites_.size())) {
		ImGui::TextDisabled("Select a sprite to preview.");
		return;
	}

	const auto& sprite = sprites_[selected_sprite_];
	ImGui::Text("%s", sprite.name.c_str());
	ImGui::Text("Position: %d, %d", sprite.x, sprite.y);
	ImGui::Text("Size: %d x %d", sprite.width, sprite.height);

	if (!loaded_image_ || gpu_texture_id_ == engine::rendering::INVALID_TEXTURE) {
		return;
	}

	auto* gl_tex = engine::rendering::GetGLTexture(gpu_texture_id_);
	if (gl_tex == nullptr) {
		return;
	}

	const float img_w = static_cast<float>(loaded_image_->width);
	const float img_h = static_cast<float>(loaded_image_->height);

	// Calculate UVs (flipped vertically for OpenGL)
	const float u0 = static_cast<float>(sprite.x) / img_w;
	const float u1 = static_cast<float>(sprite.x + sprite.width) / img_w;
	const float v0 = 1.0f - static_cast<float>(sprite.y) / img_h;
	const float v1 = 1.0f - static_cast<float>(sprite.y + sprite.height) / img_h;

	const auto imgui_tex = static_cast<ImTextureID>(static_cast<uintptr_t>(gl_tex->handle));

	constexpr float MAX_PREVIEW_SIZE = 128.0f;
	float preview_w = static_cast<float>(sprite.width);
	float preview_h = static_cast<float>(sprite.height);

	// Scale to fit preview area
	if (preview_w > MAX_PREVIEW_SIZE || preview_h > MAX_PREVIEW_SIZE) {
		const float scale = MAX_PREVIEW_SIZE / std::max(preview_w, preview_h);
		preview_w *= scale;
		preview_h *= scale;
	} else {
		// Scale up small sprites for visibility
		const float scale = std::min(MAX_PREVIEW_SIZE / std::max(preview_w, preview_h), 4.0f);
		preview_w *= scale;
		preview_h *= scale;
	}

	ImGui::Image(imgui_tex, ImVec2(preview_w, preview_h), ImVec2(u0, v0), ImVec2(u1, v1));

	// Show UV coordinates
	ImGui::TextDisabled("UV: (%.3f, %.3f) - (%.3f, %.3f)", u0, 1.0f - v0, u1, 1.0f - v1);
}

void SpriteEditorPanel::AutoGrid() {
	if (!loaded_image_) {
		status_message_ = "Load an image before using Auto Grid.";
		status_is_error_ = true;
		return;
	}

	sprites_.clear();
	selected_sprite_ = -1;

	const int cols = loaded_image_->width / grid_width_;
	const int rows = loaded_image_->height / grid_height_;

	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			SpriteRegion region;
			region.name = "sprite_" + std::to_string(row * cols + col);
			region.x = col * grid_width_;
			region.y = row * grid_height_;
			region.width = grid_width_;
			region.height = grid_height_;
			sprites_.push_back(region);
		}
	}

	status_message_ = "Created " + std::to_string(sprites_.size()) + " sprites (" + std::to_string(cols) + "x" + std::to_string(rows) + " grid)";
	status_is_error_ = false;
}

void SpriteEditorPanel::ExportAtlas() {
	if (sprites_.empty()) {
		status_message_ = "No sprites to export.";
		status_is_error_ = true;
		return;
	}

	const std::string export_path = export_path_buffer_;
	if (export_path.empty()) {
		status_message_ = "Please enter an export path.";
		status_is_error_ = true;
		return;
	}

	try {
		json j;
		j["asset_type"] = "sprite_atlas";
		j["image"] = image_path_;

		json sprites_array = json::array();
		for (const auto& sprite : sprites_) {
			json sprite_json;
			sprite_json["name"] = sprite.name;
			sprite_json["x"] = sprite.x;
			sprite_json["y"] = sprite.y;
			sprite_json["width"] = sprite.width;
			sprite_json["height"] = sprite.height;
			sprites_array.push_back(sprite_json);
		}
		j["sprites"] = sprites_array;

		if (!engine::assets::AssetManager::SaveTextFile(std::filesystem::path(export_path), j.dump(2))) {
			status_message_ = "Failed to save: " + export_path;
			status_is_error_ = true;
			return;
		}

		status_message_ = "Exported " + std::to_string(sprites_.size()) + " sprites to " + export_path;
		status_is_error_ = false;
	} catch (const std::exception& e) {
		status_message_ = std::string("Export error: ") + e.what();
		status_is_error_ = true;
	}
}

bool SpriteEditorPanel::ImportAtlas(const std::string& path) {
	try {
		auto text = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
		if (!text) {
			return false;
		}

		json j = json::parse(*text);

		// Read image path
		std::string img_path = j.value("image", "");

		// Read sprites
		std::vector<SpriteRegion> imported_sprites;
		if (j.contains("sprites") && j["sprites"].is_array()) {
			for (const auto& sprite_json : j["sprites"]) {
				SpriteRegion region;
				region.name = sprite_json.value("name", "");
				region.x = sprite_json.value("x", 0);
				region.y = sprite_json.value("y", 0);
				region.width = sprite_json.value("width", 0);
				region.height = sprite_json.value("height", 0);
				imported_sprites.push_back(region);
			}
		}

		sprites_ = std::move(imported_sprites);
		selected_sprite_ = sprites_.empty() ? -1 : 0;

		// Update image path and export path
		if (!img_path.empty()) {
			image_path_ = img_path;
			std::strncpy(image_path_buffer_, img_path.c_str(), sizeof(image_path_buffer_) - 1);
			image_path_buffer_[sizeof(image_path_buffer_) - 1] = '\0';
		}

		std::strncpy(export_path_buffer_, path.c_str(), sizeof(export_path_buffer_) - 1);
		export_path_buffer_[sizeof(export_path_buffer_) - 1] = '\0';

		status_message_ = "Loaded " + std::to_string(sprites_.size()) + " sprites from " + path;
		status_is_error_ = false;

		return true;
	} catch (const std::exception& e) {
		status_message_ = std::string("Failed to load atlas: ") + e.what();
		status_is_error_ = true;
		return false;
	}
}

void SpriteEditorPanel::OpenAtlas(const std::string& path) {
	if (ImportAtlas(path)) {
		SetVisible(true);
		// Defer image loading until next Render() which has engine access
		if (!image_path_.empty()) {
			pending_image_load_ = true;
			pending_image_path_ = image_path_;
		}
	}
}

} // namespace editor
