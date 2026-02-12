#pragma once

#include "asset_browser_panel.h"
#include "editor_callbacks.h"
#include "editor_panel.h"

import engine;
import glm;

namespace editor {

/**
 * @brief Entity properties inspector panel
 *
 * Displays and allows editing of components on the selected entity.
 * When no entity is selected but an asset is, shows asset properties.
 * When nothing is selected, shows scene-level properties.
 */
class PropertiesPanel : public EditorPanel {
public:
	PropertiesPanel() = default;
	~PropertiesPanel() override = default;

	[[nodiscard]] std::string_view GetPanelName() const override;

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
	 * @param scene_active_camera The scene's intended active camera (not editor camera)
	 */
	void
	Render(engine::ecs::Entity selected_entity,
		   engine::ecs::ECSWorld& world,
		   engine::scene::Scene* scene,
		   const AssetSelection& selected_asset,
		   engine::ecs::Entity scene_active_camera);

private:
	void RenderComponentSections(engine::ecs::Entity entity, engine::scene::Scene* scene) const;
	void RenderComponentFields(
			engine::ecs::Entity entity, const engine::ecs::ComponentInfo& comp, engine::scene::Scene* scene) const;
	void RenderAddComponentButton(engine::ecs::Entity entity) const;
	void RenderSceneProperties(engine::ecs::ECSWorld& world, engine::ecs::Entity scene_active_camera) const;
	void RenderAssetProperties(engine::scene::Scene* scene, const AssetSelection& selected_asset) const;

	EditorCallbacks callbacks_;
	mutable bool physics_backend_changed_{false};
	mutable engine::scene::Scene* last_scene_{nullptr};
};

} // namespace editor
