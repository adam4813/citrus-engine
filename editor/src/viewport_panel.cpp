#include "viewport_panel.h"

#include <imgui.h>

namespace editor {

void ViewportPanel::Render(bool is_running) {
	if (!is_visible_)
		return;

	ImGui::Begin("Viewport", &is_visible_);

	const ImVec2 content_size = ImGui::GetContentRegionAvail();

	ImGui::BeginChild("ViewportContent", content_size, true);

	const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

	RenderGrid(cursor_pos, content_size);

	// Draw center text
	const auto text = "2D Scene Viewport";
	const ImVec2 text_size = ImGui::CalcTextSize(text);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddText(
			ImVec2(cursor_pos.x + (content_size.x - text_size.x) / 2, cursor_pos.y + (content_size.y - text_size.y) / 2),
			IM_COL32(100, 100, 100, 255),
			text);

	if (is_running) {
		RenderPlayModeIndicator(cursor_pos);
	}

	ImGui::EndChild();

	ImGui::End();
}

void ViewportPanel::RenderGrid(const ImVec2& cursor_pos, const ImVec2& content_size) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw background
	draw_list->AddRectFilled(
			cursor_pos, ImVec2(cursor_pos.x + content_size.x, cursor_pos.y + content_size.y), IM_COL32(30, 30, 30, 255));

	// Draw grid lines
	constexpr float grid_size = 50.0f;
	constexpr ImU32 grid_color = IM_COL32(50, 50, 50, 255);

	for (float x = 0; x < content_size.x; x += grid_size) {
		draw_list->AddLine(
				ImVec2(cursor_pos.x + x, cursor_pos.y), ImVec2(cursor_pos.x + x, cursor_pos.y + content_size.y), grid_color);
	}

	for (float y = 0; y < content_size.y; y += grid_size) {
		draw_list->AddLine(
				ImVec2(cursor_pos.x, cursor_pos.y + y), ImVec2(cursor_pos.x + content_size.x, cursor_pos.y + y), grid_color);
	}
}

void ViewportPanel::RenderPlayModeIndicator(const ImVec2& cursor_pos) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const auto play_text = "PLAYING";
	const ImVec2 play_text_size = ImGui::CalcTextSize(play_text);

	draw_list->AddRectFilled(
			ImVec2(cursor_pos.x + 5, cursor_pos.y + 5),
			ImVec2(cursor_pos.x + play_text_size.x + 15, cursor_pos.y + play_text_size.y + 15),
			IM_COL32(0, 100, 0, 200));
	draw_list->AddText(ImVec2(cursor_pos.x + 10, cursor_pos.y + 10), IM_COL32(255, 255, 255, 255), play_text);
}

} // namespace editor
