#pragma once

#include <imgui.h>

namespace editor {

/**
 * @brief 2D scene viewport panel
 *
 * Displays the scene view with grid and play mode indicator.
 */
class ViewportPanel {
public:
	ViewportPanel() = default;
	~ViewportPanel() = default;

	/**
	 * @brief Render the viewport panel
	 * @param is_running Whether the scene is in play mode
	 */
	void Render(bool is_running);

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
	void RenderGrid(const ImVec2& cursor_pos, const ImVec2& content_size);
	void RenderPlayModeIndicator(const ImVec2& cursor_pos);

	bool is_visible_ = true;
};

} // namespace editor
