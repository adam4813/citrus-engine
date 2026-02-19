#pragma once

#include <imgui.h>
#include <string>
#include <string_view>

namespace editor {

class AssetEditorRegistry;

/**
 * @brief Base class for all editor panels
 *
 * Provides shared behavior for editor panels:
 * - Visibility management (show/hide, View menu integration)
 * - Asset handler registration (optional, for panels that open asset files)
 * - Dirty state tracking (unsaved changes)
 * - Panel name for window titles and menu entries
 *
 * Panels inherit from this and implement GetPanelName(). The Render() method
 * signature varies per panel (different data dependencies), so it is NOT
 * part of this base class.
 */
class EditorPanel {
public:
	virtual ~EditorPanel() = default;

	/**
	 * @brief Get the display name of this panel (used for window title and View menu)
	 */
	[[nodiscard]] virtual std::string_view GetPanelName() const = 0;

	/**
	 * @brief Register asset type handlers this panel can open
	 *
	 * Override in panels that handle asset files. Default does nothing.
	 * Called once during editor initialization.
	 */
	virtual void RegisterAssetHandlers(AssetEditorRegistry& /*registry*/) {}

	/**
	 * @brief Called after the engine and OpenGL context are fully initialized
	 *
	 * Override in panels that need to perform GL-dependent initialization
	 * (e.g., create textures, set up framebuffers). The engine and rendering
	 * context are guaranteed to be ready when this is called.
	 *
	 * Called once from EditorScene::Initialize(), after node types are
	 * registered and the engine is fully set up.
	 */
	virtual void OnInitialized() {}

	// -- Visibility --

	[[nodiscard]] bool IsVisible() const { return is_visible_; }
	void SetVisible(bool visible) { is_visible_ = visible; }
	bool& VisibleRef() { return is_visible_; }

	// -- Dirty state --

	[[nodiscard]] bool IsDirty() const { return is_dirty_; }
	void SetDirty(bool dirty = true) { is_dirty_ = dirty; }

	// -- View menu integration --

	/**
	 * @brief Render this panel's entry in the View menu
	 *
	 * Call from EditorScene::RenderMenuBar() inside the View menu block.
	 */
	void RenderViewMenuItem() {
		ImGui::MenuItem(std::string(GetPanelName()).c_str(), nullptr, &is_visible_);
	}

protected:
	EditorPanel() = default;

	/**
	 * @brief Begin the ImGui window for this panel
	 *
	 * Checks visibility and calls ImGui::Begin with the panel name.
	 * Returns false if the panel should not render (not visible).
	 * Caller must call EndPanel() if this returns true.
	 */
	bool BeginPanel(ImGuiWindowFlags flags = 0) {
		if (!is_visible_) return false;
		std::string title(GetPanelName());
		if (is_dirty_) {
			title += "*";
		}
		ImGui::Begin(title.c_str(), &is_visible_, flags);
		return true;
	}

	/**
	 * @brief End the ImGui window for this panel
	 */
	static void EndPanel() {
		ImGui::End();
	}

private:
	bool is_visible_ = false;
	bool is_dirty_ = false;
};

} // namespace editor
