#pragma once

#include "asset_browser_panel.h"
#include "editor_callbacks.h"

import engine;

namespace editor {

/**
 * @brief Entity properties inspector panel
 *
 * Displays and allows editing of components on the selected entity.
 * When no entity is selected but an asset is, shows asset properties.
 * When nothing is selected, shows scene-level properties.
 */
class PropertiesPanel {
public:
	PropertiesPanel() = default;
	~PropertiesPanel() = default;

	/**
	 * @brief Set callbacks for panel events
	 */
	void SetCallbacks(const EditorCallbacks& callbacks);

	/**
	 * @brief Render the properties panel
	 * @param selected_entity The entity to inspect (may be invalid)
	 * @param world The ECS world (for scene-level properties)
	 * @param scene The current scene (for asset editing)
	 * @param selected_asset Currently selected asset (if any) 
	 */
	void
	Render(engine::ecs::Entity selected_entity,
		   engine::ecs::ECSWorld& world,
		   engine::scene::Scene* scene,
		   const AssetSelection& selected_asset);

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
	void RenderComponentSections(engine::ecs::Entity entity, engine::scene::Scene* scene) const;
	void RenderComponentFields(engine::ecs::Entity entity, const engine::ecs::ComponentInfo& comp, engine::scene::Scene* scene) const;
	void RenderAddComponentButton(engine::ecs::Entity entity) const;
	void RenderSceneProperties(engine::ecs::ECSWorld& world) const;
	void RenderAssetProperties(engine::scene::Scene* scene, const AssetSelection& selected_asset) const;

	EditorCallbacks callbacks_;
	bool is_visible_ = true;
};

} // namespace editor
