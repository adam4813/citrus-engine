#pragma once

#include <imgui.h>
#include <string>

import engine;

/**
 * @brief UI Element Inspector - Interactive box model widget for editing UI elements
 *
 * Combines concepts from Unity's RectTransform and Chrome DevTools' box model:
 * - Visual nested box diagram showing margin/border/padding/content with inline editing
 * - Clickable anchor grid (3x3) with stretch buttons on right/bottom edges
 * - Component state visualization (layout, constraints, scroll)
 *
 * Layout:
 * ```
 * +------------------+  +-------------+---+
 * |     Box Model    |  | TL | TC | TR| ^ |
 * |  [border/pad/    |  +----+----+---+ | |
 * |   content]       |  | ML | C  | MR| | |  <- Stretch V spans height
 * |                  |  +----+----+---+ | |
 * |  W x H @ (x,y)   |  | BL | BC | BR| v |
 * +------------------+  +----+----+---+---+
 *                       |<-->|<-->|<->| X |  <- Stretch H spans width, Fill in corner
 * ```
 *
 * # Usage with UIDebugVisualizer (recommended)
 *
 * @code{.cpp}
 * UIDebugVisualizer visualizer;
 * UIElementInspector inspector;
 * 
 * // Setup click-to-select via visualizer
 * visualizer.SetupClickToSelect(root_element.get());
 *
 * // In your update loop:
 * selected_element_ = const_cast<UIElement*>(visualizer.GetSelectedElement());
 *
 * // In your ImGui rendering:
 * ImGui::Begin("Inspector");
 * if (inspector.Render(selected_element_)) {
 *     selected_element_->Update();
 * }
 * ImGui::End();
 * @endcode
 */
class UIElementInspector {
public:
	/**
	 * @brief Render the inspector for an element
	 * @param element Element to inspect/edit (can be nullptr)
	 * @return true if element was modified
	 */
	bool Render(engine::ui::UIElement* element) {
		if (!element) {
			ImGui::TextDisabled("Click an element to select it");
			return false;
		}

		bool modified = false;

		// Element type and basic info
		ImGui::Text("Type: %s", GetElementTypeName(element).c_str());
		ImGui::Separator();

		// Side-by-side: Box Model | Anchor Widget
		modified |= RenderSideBySide(element);
		ImGui::Separator();

		// Components section
		if (ImGui::CollapsingHeader("Components")) {
			RenderComponentsSection(element);
		}

		return modified;
	}

private:
	// Cached constraint state for the anchor widget
	struct AnchorState {
		bool left = false;
		bool right = false;
		bool top = false;
		bool bottom = false;
		float left_value = 0.0f;
		float right_value = 0.0f;
		float top_value = 0.0f;
		float bottom_value = 0.0f;
	};
	AnchorState anchor_state_;

	// Anchor preset enum (must be before methods that use it)
	enum class AnchorPreset {
		TopLeft,
		TopCenter,
		TopRight,
		MiddleLeft,
		Center,
		MiddleRight,
		BottomLeft,
		BottomCenter,
		BottomRight,
		StretchHorizontal,
		StretchVertical,
		Fill
	};

	/**
	 * @brief Render box model and anchor widget side-by-side
	 */
	bool RenderSideBySide(engine::ui::UIElement* element) {
		bool modified = false;

		// Load anchor state once at the start (used by both box model and anchor widget)
		LoadAnchorState(element);

		// Use columns for side-by-side layout
		ImGui::Columns(2, "inspector_columns", true);

		// Left column: Box Model
		ImGui::Text("Box Model");
		modified |= RenderBoxModel(element);

		ImGui::NextColumn();

		// Right column: Anchor Widget
		ImGui::Text("Anchors");
		modified |= RenderAnchorWidget(element);

		ImGui::Columns(1);

		return modified;
	}

	/**
	 * @brief Calculate current distance from element edge to parent edge
	 */
	enum class Edge { Left, Right, Top, Bottom };

	float CalculateEdgeDistance(engine::ui::UIElement* element, Edge edge) {
		auto bounds = element->GetRelativeBounds();
		auto* parent = element->GetParent();
		if (!parent)
			return 0.0f;

		float parent_w = parent->GetWidth();
		float parent_h = parent->GetHeight();

		switch (edge) {
		case Edge::Left: return bounds.x;
		case Edge::Right: return parent_w - (bounds.x + bounds.width);
		case Edge::Top: return bounds.y;
		case Edge::Bottom: return parent_h - (bounds.y + bounds.height);
		}
		return 0.0f;
	}

	/**
	 * @brief Render visual box model diagram (Chrome DevTools style)
	 *
	 * Shows nested boxes for margin/border/padding/content with inline editable values
	 * and inline anchor toggle checkboxes inside the content area.
	 */
	bool RenderBoxModel(engine::ui::UIElement* element) {
		using namespace engine::ui::batch_renderer;

		bool modified = false;

		// Expanded box model dimensions to accommodate anchor toggles
		const float box_width = 250.0f;
		const float box_height = 160.0f;
		const float margin_inset = 14.0f;
		const float border_inset = 32.0f;
		const float padding_inset = 50.0f;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(box_width, box_height);

		// Colors matching Chrome DevTools
		ImU32 margin_color = IM_COL32(251, 181, 121, 180); // Orange
		ImU32 border_color = IM_COL32(253, 221, 155, 180); // Yellow
		ImU32 padding_color = IM_COL32(196, 223, 173, 180); // Green
		ImU32 content_color = IM_COL32(173, 196, 223, 180); // Blue

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Draw nested boxes from outside in
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + box_width, canvas_pos.y + box_height), margin_color);
		draw_list->AddRectFilled(
				ImVec2(canvas_pos.x + margin_inset, canvas_pos.y + margin_inset),
				ImVec2(canvas_pos.x + box_width - margin_inset, canvas_pos.y + box_height - margin_inset),
				border_color);
		draw_list->AddRectFilled(
				ImVec2(canvas_pos.x + border_inset, canvas_pos.y + border_inset),
				ImVec2(canvas_pos.x + box_width - border_inset, canvas_pos.y + box_height - border_inset),
				padding_color);
		draw_list->AddRectFilled(
				ImVec2(canvas_pos.x + padding_inset, canvas_pos.y + padding_inset),
				ImVec2(canvas_pos.x + box_width - padding_inset, canvas_pos.y + box_height - padding_inset),
				content_color);

		// Reserve space for the box diagram
		ImGui::Dummy(canvas_size);

		const float center_x = canvas_pos.x + box_width / 2;
		const float center_y = canvas_pos.y + box_height / 2;
		const float input_width = 30.0f;
		const float checkbox_size = 14.0f;

		// Content area bounds
		const float content_left = canvas_pos.x + padding_inset;
		const float content_right = canvas_pos.x + box_width - padding_inset;
		const float content_top = canvas_pos.y + padding_inset;
		const float content_bottom = canvas_pos.y + box_height - padding_inset;

		if (auto* panel = dynamic_cast<engine::ui::elements::Panel*>(element)) {
			float b = panel->GetBorderWidth();
			ImGui::SetCursorScreenPos(ImVec2(center_x - input_width / 2, canvas_pos.y + margin_inset));
			ImGui::SetNextItemWidth(input_width);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));
			if (ImGui::DragFloat("##border", &b, 0.5f, 0.0f, 20.0f, "%.0f")) {
				panel->SetBorderWidth(b);
				modified = true;
			}
			ImGui::PopStyleVar();

			float p = panel->GetPadding();
			ImGui::SetCursorScreenPos(ImVec2(center_x - input_width / 2, canvas_pos.y + border_inset));
			ImGui::SetNextItemWidth(input_width);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));
			if (ImGui::DragFloat("##padding", &p, 0.5f, 0.0f, 100.0f, "%.0f")) {
				panel->SetPadding(p);
				modified = true;
			}
			ImGui::PopStyleVar();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));

		// === Top anchor toggle (horizontal: checkbox + input) OR Y position ===
		{
			float top_row_y = content_top + 4;
			float total_width = checkbox_size + 4 + input_width;
			float start_x = center_x - total_width / 2;

			ImGui::SetCursorScreenPos(ImVec2(start_x, top_row_y));
			bool top_checked = anchor_state_.top;
			if (ImGui::Checkbox("##anchor_top_check", &top_checked)) {
				if (top_checked && !anchor_state_.top) {
					anchor_state_.top_value = CalculateEdgeDistance(element, Edge::Top);
				}
				anchor_state_.top = top_checked;
				ApplyAnchorState(element);
				modified = true;
			}

			ImGui::SetCursorScreenPos(ImVec2(start_x + checkbox_size + 4, top_row_y));
			ImGui::SetNextItemWidth(input_width);
			if (anchor_state_.top) {
				if (ImGui::DragFloat("##anchor_t", &anchor_state_.top_value, 0.5f, 0.0f, 1000.0f, "%.0f")) {
					ApplyAnchorState(element);
					modified = true;
				}
			} else {
				float pos_y = element->GetRelativeBounds().y;
				if (ImGui::DragFloat("##posy", &pos_y, 1.0f, -10000.0f, 10000.0f, "%.0f")) {
					element->SetRelativePosition(element->GetRelativeBounds().x, pos_y);
					modified = true;
				}
			}
		}

		// === Bottom anchor toggle (horizontal: checkbox + input) ===
		{
			float bottom_row_y = content_bottom - checkbox_size - 4;
			float total_width = checkbox_size + 4 + input_width;
			float start_x = center_x - total_width / 2;

			ImGui::SetCursorScreenPos(ImVec2(start_x, bottom_row_y));
			bool bottom_checked = anchor_state_.bottom;
			if (ImGui::Checkbox("##anchor_bottom_check", &bottom_checked)) {
				if (bottom_checked && !anchor_state_.bottom) {
					anchor_state_.bottom_value = CalculateEdgeDistance(element, Edge::Bottom);
				}
				anchor_state_.bottom = bottom_checked;
				ApplyAnchorState(element);
				modified = true;
			}

			if (anchor_state_.bottom) {
				ImGui::SetCursorScreenPos(ImVec2(start_x + checkbox_size + 4, bottom_row_y));
				ImGui::SetNextItemWidth(input_width);
				if (ImGui::DragFloat("##anchor_b", &anchor_state_.bottom_value, 0.5f, 0.0f, 1000.0f, "%.0f")) {
					ApplyAnchorState(element);
					modified = true;
				}
			}
		}

		// === Left anchor toggle (vertical: checkbox with input below) OR X position ===
		{
			float left_col_x = content_left + 4;
			float checkbox_y = center_y - checkbox_size / 2 - 8;

			ImGui::SetCursorScreenPos(ImVec2(left_col_x, checkbox_y));
			bool left_checked = anchor_state_.left;
			if (ImGui::Checkbox("##anchor_left_check", &left_checked)) {
				if (left_checked && !anchor_state_.left) {
					anchor_state_.left_value = CalculateEdgeDistance(element, Edge::Left);
				}
				anchor_state_.left = left_checked;
				ApplyAnchorState(element);
				modified = true;
			}

			ImGui::SetCursorScreenPos(ImVec2(left_col_x - 3, checkbox_y + checkbox_size + 2));
			ImGui::SetNextItemWidth(input_width);
			if (anchor_state_.left) {
				if (ImGui::DragFloat("##anchor_l", &anchor_state_.left_value, 0.5f, 0.0f, 1000.0f, "%.0f")) {
					ApplyAnchorState(element);
					modified = true;
				}
			} else {
				float pos_x = element->GetRelativeBounds().x;
				if (ImGui::DragFloat("##posx", &pos_x, 1.0f, -10000.0f, 10000.0f, "%.0f")) {
					element->SetRelativePosition(pos_x, element->GetRelativeBounds().y);
					modified = true;
				}
			}
		}

		// === Right anchor toggle (vertical: checkbox with input below) ===
		{
			float right_col_x = content_right - checkbox_size - 4;
			float checkbox_y = center_y - checkbox_size / 2 - 8;
			ImGui::SetCursorScreenPos(ImVec2(right_col_x, checkbox_y));
			bool right_checked = anchor_state_.right;
			if (ImGui::Checkbox("##anchor_right_check", &right_checked)) {
				if (right_checked && !anchor_state_.right) {
					anchor_state_.right_value = CalculateEdgeDistance(element, Edge::Right);
				}
				anchor_state_.right = right_checked;
				ApplyAnchorState(element);
				modified = true;
			}

			if (anchor_state_.right) {
				ImGui::SetCursorScreenPos(
						ImVec2(right_col_x - input_width + checkbox_size + 3, checkbox_y + checkbox_size + 2));
				ImGui::SetNextItemWidth(input_width);
				if (ImGui::DragFloat("##anchor_r", &anchor_state_.right_value, 0.5f, 0.0f, 1000.0f, "%.0f")) {
					ApplyAnchorState(element);
					modified = true;
				}
			}
		}

		// === W x H inputs in center ===
		ImGui::SetCursorScreenPos(ImVec2(center_x - input_width - 6, center_y - 8));
		ImGui::SetNextItemWidth(input_width);
		float new_width = element->GetWidth();
		if (ImGui::DragFloat("##width", &new_width, 1.0f, 1.0f, 10000.0f, "%.0f")) {
			element->SetSize(new_width, element->GetHeight());
			modified = true;
		}

		ImGui::SetCursorScreenPos(ImVec2(center_x - 3, center_y - 6));
		ImGui::Text("x");

		ImGui::SetCursorScreenPos(ImVec2(center_x + 6, center_y - 8));
		ImGui::SetNextItemWidth(input_width);
		float new_height = element->GetHeight();
		if (ImGui::DragFloat("##height", &new_height, 1.0f, 1.0f, 10000.0f, "%.0f")) {
			element->SetSize(element->GetWidth(), new_height);
			modified = true;
		}

		ImGui::PopStyleVar();

		ImGui::Spacing();
		return modified;
	}

	/**
	 * @brief Render anchor widget as 4x4 grid with stretch buttons on edges
	 *
	 * Layout:
	 * ```
	 * +----+----+----+----+
	 * | TL | TC | TR |    |
	 * +----+----+----+  ^ |
	 * | ML |    | MR |  | |  <- Stretch V (tall button on right)
	 * +----+----+----+  v |
	 * | BL | BC | BR |    |
	 * +----+----+----+----+
	 * |     <-->     |  X |  <- Stretch H (wide) + Fill in corner
	 * +----+----+----+----+
	 * ```
	 */
	bool RenderAnchorWidget(engine::ui::UIElement* element) {
		bool modified = false;

		// Determine current preset for highlighting (state already loaded by RenderSideBySide)
		AnchorPreset current_preset = GetCurrentPreset();

		const float btn_size = 28.0f;
		const float stretch_btn_width = btn_size * 3 + 4; // 3 buttons wide
		const float stretch_btn_height = btn_size * 3 + 4; // 3 buttons tall
		const float spacing = 2.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

		// Row 1: TL, TC, TR
		if (DrawAnchorButton("TL", AnchorPreset::TopLeft, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::TopLeft);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton("TC", AnchorPreset::TopCenter, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::TopCenter);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton("TR", AnchorPreset::TopRight, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::TopRight);
			modified = true;
		}
		ImGui::SameLine();

		// Stretch V button - spans 3 rows on the right
		// We'll use a workaround: draw it as a tall button on first row
		ImVec2 stretch_v_pos = ImGui::GetCursorScreenPos();

		ImGui::NewLine();

		// Row 2: ML, C, MR
		if (DrawAnchorButton("ML", AnchorPreset::MiddleLeft, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::MiddleLeft);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton(" ", AnchorPreset::Center, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::Center);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton("MR", AnchorPreset::MiddleRight, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::MiddleRight);
			modified = true;
		}

		// Row 3: BL, BC, BR
		if (DrawAnchorButton("BL", AnchorPreset::BottomLeft, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::BottomLeft);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton("BC", AnchorPreset::BottomCenter, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::BottomCenter);
			modified = true;
		}
		ImGui::SameLine();
		if (DrawAnchorButton("BR", AnchorPreset::BottomRight, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::BottomRight);
			modified = true;
		}

		// Draw the tall Stretch V button on the right (using SetCursorScreenPos)
		ImGui::SetCursorScreenPos(stretch_v_pos);
		// Unicode arrows for stretch: ↕ (U+2195) or just use ASCII
		if (DrawAnchorButton(
					"^\n|\n|\nv", AnchorPreset::StretchVertical, current_preset, btn_size, stretch_btn_height)) {
			SetAnchorPreset(AnchorPreset::StretchVertical);
			modified = true;
		}

		// Row 4: Stretch H (wide) + Fill in corner
		if (DrawAnchorButton("<-->", AnchorPreset::StretchHorizontal, current_preset, stretch_btn_width, btn_size)) {
			SetAnchorPreset(AnchorPreset::StretchHorizontal);
			modified = true;
		}
		ImGui::SameLine();
		// Fill button in corner - use X with arrows or just "+"
		if (DrawAnchorButton("+", AnchorPreset::Fill, current_preset, btn_size, btn_size)) {
			SetAnchorPreset(AnchorPreset::Fill);
			modified = true;
		}

		ImGui::PopStyleVar(2);

		// Apply changes to element
		if (modified) {
			ApplyAnchorState(element);
		}

		ImGui::Dummy(ImVec2(0, 60.0f)); // Spacing for the box models extra height
		return modified;
	}

	/**
	 * @brief Draw a anchor button with active state highlighting
	 */
	bool DrawAnchorButton(const char* label, AnchorPreset preset, AnchorPreset current, float w, float h) {
		bool is_active = (preset == current);

		if (is_active) {
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(245, 166, 66, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 186, 86, 255));
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
		}

		bool clicked = ImGui::Button(label, ImVec2(w, h));

		if (is_active) {
			ImGui::PopStyleColor(3);
		}

		return clicked;
	}

	/**
	 * @brief Determine current preset from anchor state
	 */
	AnchorPreset GetCurrentPreset() const {
		bool l = anchor_state_.left, r = anchor_state_.right;
		bool t = anchor_state_.top, b = anchor_state_.bottom;

		if (l && r && t && b)
			return AnchorPreset::Fill;
		if (l && r && !t && !b)
			return AnchorPreset::StretchHorizontal;
		if (!l && !r && t && b)
			return AnchorPreset::StretchVertical;
		if (l && t && !r && !b)
			return AnchorPreset::TopLeft;
		if (!l && t && !r && !b)
			return AnchorPreset::TopCenter;
		if (r && t && !l && !b)
			return AnchorPreset::TopRight;
		if (l && !t && !r && !b)
			return AnchorPreset::MiddleLeft;
		if (r && !t && !l && !b)
			return AnchorPreset::MiddleRight;
		if (l && b && !r && !t)
			return AnchorPreset::BottomLeft;
		if (!l && b && !r && !t)
			return AnchorPreset::BottomCenter;
		if (r && b && !l && !t)
			return AnchorPreset::BottomRight;

		return AnchorPreset::Center; // Default: no anchors
	}

	/**
	 * @brief Render components attached to element
	 */
	void RenderComponentsSection(engine::ui::UIElement* element) {
		// Check for LayoutComponent (typically only on containers, but check anyway)
		if (auto* layout = element->GetComponent<engine::ui::components::LayoutComponent>()) {
			ImGui::BulletText("LayoutComponent: Active");
		}

		// Check for ConstraintComponent
		if (auto* constraint = element->GetComponent<engine::ui::components::ConstraintComponent>()) {
			ImGui::BulletText("ConstraintComponent: Active");
			const auto& anchor = constraint->GetAnchor();
			// Could show more details here
		}

		// Check for ScrollComponent
		if (auto* scroll = element->GetComponent<engine::ui::components::ScrollComponent>()) {
			ImGui::BulletText("ScrollComponent: Active");
			auto& state = scroll->GetState();
			ImGui::Text("  Scroll: (%.0f, %.0f)", state.GetScrollX(), state.GetScrollY());
			ImGui::Text("  Content: %.0f x %.0f", state.GetContentWidth(), state.GetContentHeight());
		}

		if (!element->GetComponent<engine::ui::components::LayoutComponent>()
			&& !element->GetComponent<engine::ui::components::ConstraintComponent>()
			&& !element->GetComponent<engine::ui::components::ScrollComponent>()) {
			ImGui::TextDisabled("No components");
		}
	}

	// === Helper methods ===

	void SetAnchorPreset(AnchorPreset preset) {
		// Reset all
		anchor_state_ = AnchorState{};
		const float margin = 10.0f;

		switch (preset) {
		case AnchorPreset::TopLeft:
			anchor_state_.left = anchor_state_.top = true;
			anchor_state_.left_value = anchor_state_.top_value = margin;
			break;
		case AnchorPreset::TopCenter:
			anchor_state_.top = true;
			anchor_state_.top_value = margin;
			break;
		case AnchorPreset::TopRight:
			anchor_state_.right = anchor_state_.top = true;
			anchor_state_.right_value = anchor_state_.top_value = margin;
			break;
		case AnchorPreset::MiddleLeft:
			anchor_state_.left = true;
			anchor_state_.left_value = margin;
			break;
		case AnchorPreset::Center:
			// No anchors = centered
			break;
		case AnchorPreset::MiddleRight:
			anchor_state_.right = true;
			anchor_state_.right_value = margin;
			break;
		case AnchorPreset::BottomLeft:
			anchor_state_.left = anchor_state_.bottom = true;
			anchor_state_.left_value = anchor_state_.bottom_value = margin;
			break;
		case AnchorPreset::BottomCenter:
			anchor_state_.bottom = true;
			anchor_state_.bottom_value = margin;
			break;
		case AnchorPreset::BottomRight:
			anchor_state_.right = anchor_state_.bottom = true;
			anchor_state_.right_value = anchor_state_.bottom_value = margin;
			break;
		case AnchorPreset::StretchHorizontal:
			anchor_state_.left = anchor_state_.right = true;
			anchor_state_.left_value = anchor_state_.right_value = margin;
			break;
		case AnchorPreset::StretchVertical:
			anchor_state_.top = anchor_state_.bottom = true;
			anchor_state_.top_value = anchor_state_.bottom_value = margin;
			break;
		case AnchorPreset::Fill:
			anchor_state_.left = anchor_state_.right = anchor_state_.top = anchor_state_.bottom = true;
			anchor_state_.left_value = anchor_state_.right_value = margin;
			anchor_state_.top_value = anchor_state_.bottom_value = margin;
			break;
		}
	}

	void LoadAnchorState(engine::ui::UIElement* element) {
		// Reset state first
		anchor_state_ = AnchorState{};

		// Get existing ConstraintComponent (now available on any UIElement)
		auto* constraint = element->GetComponent<engine::ui::components::ConstraintComponent>();
		if (!constraint)
			return;

		const auto& anchor = constraint->GetAnchor();
		auto [left, right, top, bottom] = anchor.GetValues();

		anchor_state_.left = left.has_value();
		anchor_state_.right = right.has_value();
		anchor_state_.top = top.has_value();
		anchor_state_.bottom = bottom.has_value();

		if (left)
			anchor_state_.left_value = *left;
		if (right)
			anchor_state_.right_value = *right;
		if (top)
			anchor_state_.top_value = *top;
		if (bottom)
			anchor_state_.bottom_value = *bottom;
	}

	void ApplyAnchorState(engine::ui::UIElement* element) {
		// Build anchor from state
		engine::ui::components::Anchor anchor;
		if (anchor_state_.left)
			anchor.SetLeft(anchor_state_.left_value);
		if (anchor_state_.right)
			anchor.SetRight(anchor_state_.right_value);
		if (anchor_state_.top)
			anchor.SetTop(anchor_state_.top_value);
		if (anchor_state_.bottom)
			anchor.SetBottom(anchor_state_.bottom_value);

		// Get or create ConstraintComponent (now available on any UIElement)
		auto* constraint = element->GetComponent<engine::ui::components::ConstraintComponent>();
		if (constraint) {
			constraint->SetAnchor(anchor);
		}
		else {
			element->AddComponent<engine::ui::components::ConstraintComponent>(anchor);
		}
	}

	std::string GetElementTypeName(const engine::ui::UIElement* element) {
		const std::string type_name = typeid(*element).name();

#ifdef _MSC_VER
		size_t last_colon = type_name.find_last_of(':');
		if (last_colon != std::string::npos) {
			return type_name.substr(last_colon + 1);
		}
		if (type_name.find("class ") == 0) {
			return type_name.substr(6);
		}
#endif
		return "UIElement";
	}
};
