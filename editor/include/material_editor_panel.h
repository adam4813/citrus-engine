#pragma once

#include "editor_panel.h"
#include "file_dialog.h"

#include <memory>
#include <optional>
#include <string>

import engine;

namespace editor {

/**
 * @brief Editor panel for creating and editing .material.json files
 *
 * Opens via asset browser double-click on .material.json or New Material context menu.
 * Edits PBR material properties: shader, colors, texture slots, scalars.
 */
class MaterialEditorPanel : public EditorPanel {
public:
	MaterialEditorPanel() = default;

	[[nodiscard]] std::string_view GetPanelName() const override { return "Material Editor"; }
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

	void Render();

	void OpenMaterial(const std::string& path);
	bool SaveMaterial();

private:
	void RenderToolbar();
	void RenderMaterialProperties();

	// Current material data loaded from file
	std::shared_ptr<engine::scene::MaterialAssetInfo> material_;
	std::string current_file_path_;

	// File dialogs
	std::optional<FileDialogPopup> save_dialog_;
};

} // namespace editor
