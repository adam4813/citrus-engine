#pragma once

#include "editor_callbacks.h"

#include <memory>
#include <string>

import engine;

namespace editor {
/**
 * @brief Info about the currently selected asset
 */
struct AssetSelection {
	engine::scene::AssetType type{};
	std::string name;

	void Clear() { name.clear(); }

	[[nodiscard]] bool IsValid() const { return !name.empty(); }
};

/**
 * @brief Asset browser panel for viewing and managing scene assets
 *
 * Displays assets organized by type (Shaders, Textures, etc.) in a tree view.
 * Uses AssetRegistry field metadata to render asset properties dynamically.
 * Supports selection for editing in Properties panel and context menus for
 * creating new assets.
 */
class AssetBrowserPanel {
public:
	AssetBrowserPanel() = default;
	~AssetBrowserPanel() = default;

	/**
	 * @brief Set callbacks for panel events
	 */
	void SetCallbacks(const EditorCallbacks& callbacks);

	/**
	 * @brief Render the asset browser panel
	 * @param scene The current scene to display assets from
	 * @param selected_asset Currently selected asset (for highlighting)
	 */
	void Render(engine::scene::Scene* scene, const AssetSelection& selected_asset);

	/**
	 * @brief Check if panel is visible
	 */
	[[nodiscard]] bool IsVisible() const { return is_visible_; }

	/**
	 * @brief Set panel visibility
	 */
	void SetVisible(bool visible) { is_visible_ = visible; }

	/**
	 * @brief Get mutable reference to visibility (for ImGui::MenuItem binding)
	 */
	bool& VisibleRef() { return is_visible_; }

private:
	/**
	 * @brief Render assets for a specific asset type
	 */
	void RenderAssetCategory(
			engine::scene::Scene* scene,
			const engine::scene::AssetTypeInfo& type_info,
			const AssetSelection& selected_asset) const;

	EditorCallbacks callbacks_;
	bool is_visible_ = true;
};

} // namespace editor
