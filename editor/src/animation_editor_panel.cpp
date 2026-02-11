#include "animation_editor_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <variant>

namespace editor {

AnimationEditorPanel::AnimationEditorPanel() {
	clip_.name = "New Animation";
	clip_.duration = 5.0f;
	clip_.looping = false;
}

AnimationEditorPanel::~AnimationEditorPanel() = default;

std::string_view AnimationEditorPanel::GetPanelName() const { return "Animation Editor"; }

void AnimationEditorPanel::Render() {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Animation Editor", &VisibleRef(), ImGuiWindowFlags_MenuBar);

	RenderMenuBar();
	RenderToolbar();

	// Split view: timeline ruler at top, tracks below
	ImGui::BeginChild("Timeline", ImVec2(0, TIMELINE_HEIGHT), true, ImGuiWindowFlags_NoScrollbar);
	RenderTimeline();
	ImGui::EndChild();

	ImGui::BeginChild("Tracks", ImVec2(0, 0), true);
	RenderTracks();
	ImGui::EndChild();

	// Dialogs
	if (show_save_dialog_) {
		ImGui::OpenPopup("Save Animation");
		show_save_dialog_ = false;
	}

	if (ImGui::BeginPopupModal("Save Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file path:");
		ImGui::InputText("##savepath", save_path_buffer_, sizeof(save_path_buffer_));

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			SaveClip(save_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (show_load_dialog_) {
		ImGui::OpenPopup("Load Animation");
		show_load_dialog_ = false;
	}

	if (ImGui::BeginPopupModal("Load Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file path:");
		ImGui::InputText("##loadpath", load_path_buffer_, sizeof(load_path_buffer_));

		if (ImGui::Button("Load", ImVec2(120, 0))) {
			LoadClip(load_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (show_add_track_dialog_) {
		ImGui::OpenPopup("Add Track");
		show_add_track_dialog_ = false;
	}

	if (ImGui::BeginPopupModal("Add Track", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Property name:");
		ImGui::InputText("##trackname", new_track_name_buffer_, sizeof(new_track_name_buffer_));
		ImGui::TextDisabled("Examples: position.x, rotation.z, scale.x, color.r");

		if (ImGui::Button("Add", ImVec2(120, 0))) {
			if (new_track_name_buffer_[0] != '\0') {
				AddTrack(new_track_name_buffer_);
				new_track_name_buffer_[0] = '\0';
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Keyframe property editor (side panel or modal)
	if (show_keyframe_editor_ && selected_track_ >= 0 && selected_keyframe_ >= 0) {
		ImGui::OpenPopup("Keyframe Properties");
		show_keyframe_editor_ = false;
	}

	if (ImGui::BeginPopupModal("Keyframe Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (selected_track_ >= 0 && selected_keyframe_ >= 0
			&& selected_track_ < static_cast<int>(clip_.tracks.size())) {
			auto& track = clip_.tracks[selected_track_];
			if (selected_keyframe_ < static_cast<int>(track.keyframes.size())) {
				auto& keyframe = track.keyframes[selected_keyframe_];

				// Time
				float time = keyframe.time;
				if (ImGui::DragFloat("Time", &time, 0.01f, 0.0f, clip_.duration)) {
					keyframe.time = std::clamp(time, 0.0f, clip_.duration);
				}

				// Value (only support float for now)
				if (std::holds_alternative<float>(keyframe.value)) {
					float value = std::get<float>(keyframe.value);
					if (ImGui::DragFloat("Value", &value, 0.01f)) {
						keyframe.value = value;
					}
				}

				// Interpolation mode
				int interp_mode = static_cast<int>(track.interpolation);
				const char* interp_items[] = {"Step", "Linear", "Cubic"};
				if (ImGui::Combo("Interpolation", &interp_mode, interp_items, 3)) {
					track.interpolation = static_cast<engine::animation::InterpolationMode>(interp_mode);
				}
			}
		}

		if (ImGui::Button("Close", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::End();

	// Update playback
	if (is_playing_ && !is_paused_) {
		UpdatePlayback(ImGui::GetIO().DeltaTime);
	}
}

void AnimationEditorPanel::RenderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewClip();
			}
			if (ImGui::MenuItem("Save")) {
				save_path_buffer_[0] = '\0';
				show_save_dialog_ = true;
			}
			if (ImGui::MenuItem("Load")) {
				load_path_buffer_[0] = '\0';
				show_load_dialog_ = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Add Track")) {
				new_track_name_buffer_[0] = '\0';
				show_add_track_dialog_ = true;
			}
			if (ImGui::MenuItem("Remove Track", nullptr, false, selected_track_ >= 0)) {
				RemoveTrack(selected_track_);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Add Keyframe", nullptr, false, selected_track_ >= 0)) {
				AddKeyframe(selected_track_, current_time_);
			}
			if (ImGui::MenuItem("Delete Keyframe", nullptr, false, selected_track_ >= 0 && selected_keyframe_ >= 0)) {
				DeleteKeyframe(selected_track_, selected_keyframe_);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void AnimationEditorPanel::RenderToolbar() {
	// Transport controls
	if (ImGui::Button(is_playing_ && !is_paused_ ? "Pause" : "Play")) {
		if (is_playing_ && !is_paused_) {
			Pause();
		}
		else {
			Play();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		Stop();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Loop", &is_looping_);

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// Time display
	int current_frame = TimeToFrame(current_time_);
	int total_frames = TimeToFrame(clip_.duration);
	ImGui::Text("Frame: %d / %d", current_frame, total_frames);

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// FPS control
	ImGui::SetNextItemWidth(80.0f);
	ImGui::DragFloat("FPS", &fps_, 1.0f, 1.0f, 120.0f, "%.0f");

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// Duration control
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::DragFloat("Duration", &clip_.duration, 0.1f, 0.1f, 60.0f, "%.1f s")) {
		current_time_ = std::min(current_time_, clip_.duration);
	}
}

void AnimationEditorPanel::RenderTimeline() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	timeline_canvas_p0_ = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	if (canvas_sz.x <= 0.0f || canvas_sz.y <= 0.0f) {
		return;
	}
	timeline_canvas_p1_ = ImVec2(timeline_canvas_p0_.x + canvas_sz.x, timeline_canvas_p0_.y + canvas_sz.y);

	// Background
	draw_list->AddRectFilled(timeline_canvas_p0_, timeline_canvas_p1_, IM_COL32(40, 40, 40, 255));

	// Clip to canvas
	draw_list->PushClipRect(timeline_canvas_p0_, timeline_canvas_p1_, true);

	// Draw frame numbers and tick marks
	const int total_frames = TimeToFrame(clip_.duration);
	const float frame_spacing = timeline_zoom_;

	for (int frame = 0; frame <= total_frames; ++frame) {
		const float time = FrameToTime(frame);
		const float x = TimeToX(time);

		if (x < timeline_canvas_p0_.x || x > timeline_canvas_p1_.x) {
			continue;
		}

		// Draw tick marks every frame, larger every 5 frames
		const bool is_major = (frame % 5 == 0);
		const float tick_height = is_major ? 15.0f : 8.0f;
		const ImU32 tick_color = is_major ? IM_COL32(150, 150, 150, 255) : IM_COL32(100, 100, 100, 255);
		draw_list->AddLine(
				ImVec2(x, timeline_canvas_p1_.y - tick_height), ImVec2(x, timeline_canvas_p1_.y), tick_color);

		// Draw frame number for major ticks
		if (is_major) {
			char label[32];
			snprintf(label, sizeof(label), "%d", frame);
			const ImVec2 text_size = ImGui::CalcTextSize(label);
			draw_list->AddText(
					ImVec2(x - text_size.x * 0.5f, timeline_canvas_p0_.y + 5.0f), IM_COL32(200, 200, 200, 255), label);
		}
	}

	// Draw playhead
	const float playhead_x = TimeToX(current_time_);
	draw_list->AddLine(
			ImVec2(playhead_x, timeline_canvas_p0_.y),
			ImVec2(playhead_x, timeline_canvas_p1_.y),
			IM_COL32(255, 100, 100, 255),
			2.0f);

	// Playhead handle (triangle at top)
	const float handle_size = 8.0f;
	const ImVec2 p1 = ImVec2(playhead_x, timeline_canvas_p0_.y);
	const ImVec2 p2 = ImVec2(playhead_x - handle_size, timeline_canvas_p0_.y + handle_size);
	const ImVec2 p3 = ImVec2(playhead_x + handle_size, timeline_canvas_p0_.y + handle_size);
	draw_list->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 100, 100, 255));

	draw_list->PopClipRect();

	// Handle playhead dragging
	ImGui::SetCursorScreenPos(timeline_canvas_p0_);
	ImGui::InvisibleButton("timeline", canvas_sz);

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		is_dragging_playhead_ = true;
		const float mouse_x = ImGui::GetMousePos().x;
		current_time_ = std::clamp(XToTime(mouse_x), 0.0f, clip_.duration);
	}
	else if (is_dragging_playhead_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		is_dragging_playhead_ = false;
	}

	// Click to set playhead
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !is_dragging_playhead_) {
		const float mouse_x = ImGui::GetMousePos().x;
		current_time_ = std::clamp(XToTime(mouse_x), 0.0f, clip_.duration);
	}

	// Mouse wheel zoom
	if (ImGui::IsItemHovered()) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			const float zoom_delta = wheel * ZOOM_SPEED;
			timeline_zoom_ = std::clamp(timeline_zoom_ + zoom_delta, MIN_ZOOM, MAX_ZOOM);
		}
	}
}

void AnimationEditorPanel::RenderTracks() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	tracks_canvas_p0_ = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	if (canvas_sz.x <= 0.0f || canvas_sz.y <= 0.0f) {
		return;
	}
	tracks_canvas_p1_ = ImVec2(tracks_canvas_p0_.x + canvas_sz.x, tracks_canvas_p0_.y + canvas_sz.y);

	// Background
	draw_list->AddRectFilled(tracks_canvas_p0_, tracks_canvas_p1_, IM_COL32(35, 35, 35, 255));

	draw_list->PushClipRect(tracks_canvas_p0_, tracks_canvas_p1_, true);

	// Track rendering
	hovered_track_ = -1;
	hovered_keyframe_ = -1;
	const ImVec2 mouse_pos = ImGui::GetMousePos();

	float y_offset = 0.0f;
	for (int track_idx = 0; track_idx < static_cast<int>(clip_.tracks.size()); ++track_idx) {
		const auto& track = clip_.tracks[track_idx];
		const ImVec2 track_start = ImVec2(tracks_canvas_p0_.x, tracks_canvas_p0_.y + y_offset);
		const ImVec2 track_end = ImVec2(tracks_canvas_p1_.x, track_start.y + TRACK_HEIGHT);

		// Track background
		const bool is_selected_track = (track_idx == selected_track_);
		const ImU32 track_bg = is_selected_track ? IM_COL32(50, 50, 70, 255) : IM_COL32(40, 40, 40, 255);
		draw_list->AddRectFilled(track_start, track_end, track_bg);

		// Track separator
		draw_list->AddLine(ImVec2(track_start.x, track_end.y), track_end, IM_COL32(60, 60, 60, 255));

		// Track label
		const ImVec2 label_pos = ImVec2(track_start.x + 5.0f, track_start.y + 8.0f);
		draw_list->AddText(label_pos, IM_COL32(200, 200, 200, 255), track.target_property.c_str());

		// Draw playhead vertical line for this track
		const float playhead_x = TimeToX(current_time_);
		draw_list->AddLine(
				ImVec2(playhead_x, track_start.y), ImVec2(playhead_x, track_end.y), IM_COL32(255, 100, 100, 128), 1.0f);

		// Draw keyframes
		for (int kf_idx = 0; kf_idx < static_cast<int>(track.keyframes.size()); ++kf_idx) {
			const auto& keyframe = track.keyframes[kf_idx];
			const float kf_x = TimeToX(keyframe.time);
			const ImVec2 kf_pos = ImVec2(kf_x, track_start.y + TRACK_HEIGHT * 0.5f);

			const bool is_selected = (track_idx == selected_track_ && kf_idx == selected_keyframe_);
			const bool is_hovered = HitTestKeyframe(mouse_pos, kf_pos);

			if (is_hovered) {
				hovered_track_ = track_idx;
				hovered_keyframe_ = kf_idx;
			}

			DrawKeyframe(draw_list, kf_pos, is_selected, is_hovered);
		}

		// Track interaction (for selection)
		if (mouse_pos.y >= track_start.y && mouse_pos.y <= track_end.y && mouse_pos.x >= tracks_canvas_p0_.x
			&& mouse_pos.x <= tracks_canvas_p1_.x) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered_keyframe_ < 0) {
				selected_track_ = track_idx;
				selected_keyframe_ = -1;
			}
		}

		y_offset += TRACK_HEIGHT;
	}

	draw_list->PopClipRect();

	// Keyframe selection
	if (hovered_keyframe_ >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		SelectKeyframe(hovered_track_, hovered_keyframe_);
	}

	// Keyframe dragging
	if (selected_track_ >= 0 && selected_keyframe_ >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		if (!is_dragging_keyframe_) {
			is_dragging_keyframe_ = true;
			drag_start_time_ = clip_.tracks[selected_track_].keyframes[selected_keyframe_].time;
		}

		const float mouse_x = ImGui::GetMousePos().x;
		const float new_time = std::clamp(XToTime(mouse_x), 0.0f, clip_.duration);
		clip_.tracks[selected_track_].keyframes[selected_keyframe_].time = new_time;
	}
	else if (is_dragging_keyframe_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		is_dragging_keyframe_ = false;
	}

	// Right-click context menu
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		context_menu_pos_ = mouse_pos;

		if (hovered_keyframe_ >= 0) {
			context_target_ = ContextTarget::Keyframe;
			context_track_ = hovered_track_;
			context_keyframe_ = hovered_keyframe_;
		}
		else if (hovered_track_ >= 0) {
			context_target_ = ContextTarget::Track;
			context_track_ = hovered_track_;
		}
		else {
			context_target_ = ContextTarget::Timeline;
		}
		ImGui::OpenPopup("TrackContextMenu");
	}

	if (ImGui::BeginPopup("TrackContextMenu")) {
		switch (context_target_) {
		case ContextTarget::Timeline:
			if (ImGui::MenuItem("Add Track")) {
				new_track_name_buffer_[0] = '\0';
				show_add_track_dialog_ = true;
			}
			break;

		case ContextTarget::Track:
			if (ImGui::MenuItem("Add Keyframe")) {
				const float time = XToTime(context_menu_pos_.x);
				AddKeyframe(context_track_, time);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Remove Track")) {
				RemoveTrack(context_track_);
			}
			break;

		case ContextTarget::Keyframe:
			if (ImGui::MenuItem("Edit Properties")) {
				SelectKeyframe(context_track_, context_keyframe_);
				show_keyframe_editor_ = true;
			}
			if (ImGui::MenuItem("Delete Keyframe")) {
				DeleteKeyframe(context_track_, context_keyframe_);
			}
			break;

		default: break;
		}

		ImGui::EndPopup();
	}

	// Handle scrolling
	ImGui::SetCursorScreenPos(tracks_canvas_p0_);
	ImGui::InvisibleButton("tracks_canvas", canvas_sz);
}

void AnimationEditorPanel::RenderKeyframeEditor() {
	// Handled in Render() via modal dialog
}

float AnimationEditorPanel::TimeToX(float time) const {
	const int frame = TimeToFrame(time);
	return tracks_canvas_p0_.x + TRACK_LABEL_WIDTH + frame * timeline_zoom_ - timeline_scroll_;
}

float AnimationEditorPanel::XToTime(float x) const {
	const float offset_x = x - tracks_canvas_p0_.x - TRACK_LABEL_WIDTH + timeline_scroll_;
	const int frame = static_cast<int>(offset_x / timeline_zoom_);
	return FrameToTime(frame);
}

int AnimationEditorPanel::TimeToFrame(float time) const { return static_cast<int>(time * fps_); }

float AnimationEditorPanel::FrameToTime(int frame) const { return static_cast<float>(frame) / fps_; }

void AnimationEditorPanel::DrawKeyframe(ImDrawList* draw_list, const ImVec2& pos, bool selected, bool hovered) {
	const float size = hovered ? KEYFRAME_SIZE * 1.3f : KEYFRAME_SIZE;
	const ImU32 color = selected ? IM_COL32(255, 200, 100, 255)
								 : (hovered ? IM_COL32(150, 255, 150, 255) : IM_COL32(100, 200, 255, 255));

	// Draw diamond shape
	const ImVec2 p1 = ImVec2(pos.x, pos.y - size);
	const ImVec2 p2 = ImVec2(pos.x + size, pos.y);
	const ImVec2 p3 = ImVec2(pos.x, pos.y + size);
	const ImVec2 p4 = ImVec2(pos.x - size, pos.y);

	draw_list->AddQuadFilled(p1, p2, p3, p4, color);
	draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(0, 0, 0, 255), 1.0f);
}

bool AnimationEditorPanel::HitTestKeyframe(const ImVec2& mouse_pos, const ImVec2& keyframe_pos) const {
	const float hit_radius = KEYFRAME_SIZE * 2.0f;
	const float dx = mouse_pos.x - keyframe_pos.x;
	const float dy = mouse_pos.y - keyframe_pos.y;
	return (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

int AnimationEditorPanel::FindKeyframeUnderMouse(int track_index, const ImVec2& mouse_pos) const {
	if (track_index < 0 || track_index >= static_cast<int>(clip_.tracks.size())) {
		return -1;
	}

	const auto& track = clip_.tracks[track_index];
	for (int i = 0; i < static_cast<int>(track.keyframes.size()); ++i) {
		const float kf_x = TimeToX(track.keyframes[i].time);
		const float kf_y = tracks_canvas_p0_.y + track_index * TRACK_HEIGHT + TRACK_HEIGHT * 0.5f;
		const ImVec2 kf_pos = ImVec2(kf_x, kf_y);

		if (HitTestKeyframe(mouse_pos, kf_pos)) {
			return i;
		}
	}

	return -1;
}

void AnimationEditorPanel::AddKeyframe(int track_index, float time) {
	if (track_index < 0 || track_index >= static_cast<int>(clip_.tracks.size())) {
		return;
	}

	auto& track = clip_.tracks[track_index];
	// Default value: 0.0f
	track.AddKeyframe(time, 0.0f);

	std::cout << "Added keyframe to track '" << track.target_property << "' at time " << time << std::endl;
}

void AnimationEditorPanel::DeleteKeyframe(int track_index, int keyframe_index) {
	if (track_index < 0 || track_index >= static_cast<int>(clip_.tracks.size())) {
		return;
	}

	auto& track = clip_.tracks[track_index];
	if (keyframe_index < 0 || keyframe_index >= static_cast<int>(track.keyframes.size())) {
		return;
	}

	track.keyframes.erase(track.keyframes.begin() + keyframe_index);
	ClearSelection();

	std::cout << "Deleted keyframe from track '" << track.target_property << "'" << std::endl;
}

void AnimationEditorPanel::SelectKeyframe(int track_index, int keyframe_index) {
	selected_track_ = track_index;
	selected_keyframe_ = keyframe_index;
}

void AnimationEditorPanel::ClearSelection() {
	selected_track_ = -1;
	selected_keyframe_ = -1;
}

void AnimationEditorPanel::AddTrack(const std::string& property_name) {
	engine::animation::AnimationTrack track;
	track.target_property = property_name;
	track.interpolation = engine::animation::InterpolationMode::Linear;
	clip_.AddTrack(std::move(track));

	std::cout << "Added track: " << property_name << std::endl;
}

void AnimationEditorPanel::RemoveTrack(int track_index) {
	if (track_index < 0 || track_index >= static_cast<int>(clip_.tracks.size())) {
		return;
	}

	const std::string track_name = clip_.tracks[track_index].target_property;
	clip_.tracks.erase(clip_.tracks.begin() + track_index);
	ClearSelection();

	std::cout << "Removed track: " << track_name << std::endl;
}

void AnimationEditorPanel::Play() {
	is_playing_ = true;
	is_paused_ = false;
}

void AnimationEditorPanel::Pause() { is_paused_ = true; }

void AnimationEditorPanel::Stop() {
	is_playing_ = false;
	is_paused_ = false;
	current_time_ = 0.0f;
}

void AnimationEditorPanel::UpdatePlayback(float delta_time) {
	current_time_ += delta_time;

	if (current_time_ >= clip_.duration) {
		if (is_looping_) {
			current_time_ = 0.0f;
		}
		else {
			current_time_ = clip_.duration;
			Stop();
		}
	}
}

void AnimationEditorPanel::NewClip() {
	clip_ = engine::animation::AnimationClip();
	clip_.name = "New Animation";
	clip_.duration = 5.0f;
	clip_.looping = false;
	current_time_ = 0.0f;
	ClearSelection();
	Stop();

	std::cout << "Created new animation clip" << std::endl;
}

bool AnimationEditorPanel::SaveClip(const std::string& path) {
	if (engine::animation::AnimationSerializer::SaveToFile(clip_, path)) {
		std::cout << "Saved animation clip to: " << path << std::endl;
		return true;
	}
	else {
		std::cerr << "Failed to save animation clip to: " << path << std::endl;
		return false;
	}
}

bool AnimationEditorPanel::LoadClip(const std::string& path) {
	auto loaded_clip = engine::animation::AnimationSerializer::LoadFromFile(path);
	if (loaded_clip) {
		clip_ = *loaded_clip;
		current_time_ = 0.0f;
		ClearSelection();
		Stop();
		std::cout << "Loaded animation clip from: " << path << std::endl;
		return true;
	}
	else {
		std::cerr << "Failed to load animation clip from: " << path << std::endl;
		return false;
	}
}

} // namespace editor
