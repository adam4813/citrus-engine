#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "editor_panel.h"
#include "file_dialog.h"
#include "grid_utils.h"

import engine;

namespace editor {

/**
 * @brief Sprite editor panel for chopping images into individual sprites
 *
 * Features:
 * - Load source image via AssetManager
 * - Configurable grid overlay
 * - Manual rectangle selection for irregular sprite regions
 * - Named sprite list with preview
 * - Export sprite atlas metadata as JSON
 */
class SpriteEditorPanel : public EditorPanel {
public:
	SpriteEditorPanel();
	~SpriteEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;

	void Render(engine::Engine& engine);

	/**
	 * @brief Register asset type handlers for this panel
	 */
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

	/**
	 * @brief Open a sprite atlas file (for asset browser integration)
	 */
	void OpenAtlas(const std::string& path);

private:
	struct SpriteRegion {
		std::string name;
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	void RenderToolbar(engine::Engine& engine);
	void RenderCanvas();
	void RenderSpriteList();
	void RenderPreview();

	void LoadSourceImage(engine::Engine& engine, const std::string& path);
	void NewAtlas();
	void AutoGrid();
	void ExportAtlas();
	bool ImportAtlas(const std::string& path);

	std::string current_file_path_;
	std::string image_path_;
	char image_path_buffer_[512] = "";
	std::shared_ptr<engine::assets::Image> loaded_image_;
	engine::rendering::TextureId gpu_texture_id_ = engine::rendering::INVALID_TEXTURE;
	std::string status_message_;
	bool status_is_error_ = false;

	GridConfig grid_;
	bool show_grid_ = true;
	float canvas_scale_ = 2.0f;

	std::vector<SpriteRegion> sprites_;
	int selected_sprite_ = -1;

	// Manual rectangle selection state
	bool is_selecting_ = false;
	ImVec2 selection_start_{0, 0};
	ImVec2 selection_end_{0, 0};

	// Export path
	char export_path_buffer_[512] = "sprites.json";

	// File dialogs
	FileDialogPopup open_dialog_{"Open Sprite Atlas", FileDialogMode::Open, {".json"}};
	FileDialogPopup save_dialog_{"Save Sprite Atlas As", FileDialogMode::Save, {".json"}};
	FileDialogPopup image_dialog_{
			"Select Source Image", FileDialogMode::Open, {".png", ".jpg", ".jpeg", ".tga", ".bmp"}};

	// Deferred image loading after OpenAtlas
	bool pending_image_load_ = false;
	std::string pending_image_path_;
};

} // namespace editor
