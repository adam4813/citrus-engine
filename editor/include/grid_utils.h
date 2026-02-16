#pragma once

#include <algorithm>
#include <imgui.h>

namespace editor {

/**
 * @brief Grid configuration for tile/sprite grid layouts.
 *
 * Shared by tileset and sprite editors for consistent grid calculations.
 */
struct GridConfig {
	int cell_width = 32;
	int cell_height = 32;
	int gap_x = 0;
	int gap_y = 0;
	int padding_x = 0;
	int padding_y = 0;

	/// Number of columns that fit in the given image width.
	[[nodiscard]] int GetColumns(int image_width) const {
		if (cell_width <= 0) return 0;
		const int usable = image_width - 2 * padding_x;
		if (usable <= 0) return 0;
		return (usable + gap_x) / (cell_width + gap_x);
	}

	/// Number of rows that fit in the given image height.
	[[nodiscard]] int GetRows(int image_height) const {
		if (cell_height <= 0) return 0;
		const int usable = image_height - 2 * padding_y;
		if (usable <= 0) return 0;
		return (usable + gap_y) / (cell_height + gap_y);
	}

	/// Pixel position of the top-left corner of cell (col, row).
	[[nodiscard]] ImVec2 CellOrigin(int col, int row) const {
		return {
			static_cast<float>(padding_x + col * (cell_width + gap_x)),
			static_cast<float>(padding_y + row * (cell_height + gap_y))};
	}

	/// Pixel position scaled for display.
	[[nodiscard]] ImVec2 CellOriginScaled(int col, int row, float scale) const {
		return {
			(padding_x + col * (cell_width + gap_x)) * scale,
			(padding_y + row * (cell_height + gap_y)) * scale};
	}

	/// Clamp all fields to valid ranges.
	void Clamp(int max_cell = 512, int max_spacing = 256) {
		cell_width = std::max(1, std::min(max_cell, cell_width));
		cell_height = std::max(1, std::min(max_cell, cell_height));
		gap_x = std::max(0, std::min(max_spacing, gap_x));
		gap_y = std::max(0, std::min(max_spacing, gap_y));
		padding_x = std::max(0, std::min(max_spacing, padding_x));
		padding_y = std::max(0, std::min(max_spacing, padding_y));
	}

	/// Convert a pixel position (relative to image origin) to grid cell coords.
	/// Returns false if the position falls in a gap rather than a cell.
	[[nodiscard]] bool PixelToCell(float px, float py, float scale, int& out_col,
		int& out_row) const {
		const float rel_x = px - padding_x * scale;
		const float rel_y = py - padding_y * scale;
		const float cell_w = (cell_width + gap_x) * scale;
		const float cell_h = (cell_height + gap_y) * scale;
		if (cell_w <= 0 || cell_h <= 0 || rel_x < 0 || rel_y < 0) return false;

		out_col = static_cast<int>(rel_x / cell_w);
		out_row = static_cast<int>(rel_y / cell_h);

		// Check if we're inside the cell area vs the gap
		const float in_cell_x = rel_x - out_col * cell_w;
		const float in_cell_y = rel_y - out_row * cell_h;
		return in_cell_x < cell_width * scale && in_cell_y < cell_height * scale;
	}

	/// Render an ImGui grid overlay on the draw list.
	void DrawGridOverlay(ImDrawList* draw_list, ImVec2 origin, int image_width,
		int image_height, float scale,
		ImU32 color = IM_COL32(255, 255, 255, 40)) const {
		const int cols = GetColumns(image_width);
		const int rows = GetRows(image_height);
		if (cols <= 0 || rows <= 0) return;

		const float pad_x = padding_x * scale;
		const float pad_y = padding_y * scale;
		const float cw = cell_width * scale;
		const float ch = cell_height * scale;
		const float gw = gap_x * scale;
		const float gh = gap_y * scale;
		const float total_h = rows * (ch + gh) - gh;
		const float total_w = cols * (cw + gw) - gw;

		// Vertical lines
		for (int c = 0; c <= cols; ++c) {
			const float gx = pad_x + c * (cw + gw);
			draw_list->AddLine(
				ImVec2(origin.x + gx, origin.y + pad_y),
				ImVec2(origin.x + gx, origin.y + pad_y + total_h), color);
			if (c < cols && gap_x > 0) {
				draw_list->AddLine(
					ImVec2(origin.x + gx + cw, origin.y + pad_y),
					ImVec2(origin.x + gx + cw, origin.y + pad_y + total_h), color);
			}
		}
		// Horizontal lines
		for (int r = 0; r <= rows; ++r) {
			const float gy = pad_y + r * (ch + gh);
			draw_list->AddLine(
				ImVec2(origin.x + pad_x, origin.y + gy),
				ImVec2(origin.x + pad_x + total_w, origin.y + gy), color);
			if (r < rows && gap_y > 0) {
				draw_list->AddLine(
					ImVec2(origin.x + pad_x, origin.y + gy + ch),
					ImVec2(origin.x + pad_x + total_w, origin.y + gy + ch), color);
			}
		}
	}
};

/// Render ImGui controls for a GridConfig and clamp values.
inline bool RenderGridConfigUI(GridConfig& config) {
	bool changed = false;
	ImGui::Text("Grid:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##grid_w", &config.cell_width, 0, 0);
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##grid_h", &config.cell_height, 0, 0);

	ImGui::Text("Gap:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##gap_x", &config.gap_x, 0, 0);
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##gap_y", &config.gap_y, 0, 0);

	ImGui::SameLine();
	ImGui::Text("Padding:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##pad_x", &config.padding_x, 0, 0);
	ImGui::SameLine();
	ImGui::Text("x");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60);
	changed |= ImGui::InputInt("##pad_y", &config.padding_y, 0, 0);

	config.Clamp();
	return changed;
}

} // namespace editor
