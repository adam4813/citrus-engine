#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "editor_panel.h"
#include "file_dialog.h"

import engine;
import glm;

namespace editor {

/**
 * @brief Timeline-based animation editor panel
 * 
 * Provides a UI for creating and editing animation clips with:
 * - Timeline ruler with frame numbers
 * - Draggable playhead scrubber
 * - Transport controls (Play, Pause, Stop, Loop)
 * - Property tracks with keyframe editing
 * - Keyframe property editor (value, interpolation mode)
 * - Zoom and scroll support
 */
class AnimationEditorPanel : public EditorPanel {
public:
	AnimationEditorPanel();
	~AnimationEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

	/**
	 * @brief Render the animation editor panel
	 */
	void Render();

	/**
	 * @brief Get the current animation clip
	 */
	engine::animation::AnimationClip& GetClip() { return clip_; }

	/**
	 * @brief Get the current animation clip (const)
	 */
	[[nodiscard]] const engine::animation::AnimationClip& GetClip() const { return clip_; }

	/**
	 * @brief Create a new empty animation clip
	 */
	void NewClip();

	/**
	 * @brief Save the current clip to a file
	 */
	bool SaveClip(const std::string& path);

	/**
	 * @brief Load a clip from a file
	 */
	bool LoadClip(const std::string& path);

private:
	// UI Rendering
	void RenderMenuBar();
	void RenderToolbar();
	void RenderTimeline();
	void RenderTracks();
	void RenderKeyframeEditor();

	// Timeline helpers
	float TimeToX(float time) const;
	float XToTime(float x) const;
	int TimeToFrame(float time) const;
	float FrameToTime(int frame) const;

	// Keyframe rendering
	void DrawKeyframe(ImDrawList* draw_list, const ImVec2& pos, bool selected, bool hovered);

	// Hit testing
	bool HitTestKeyframe(const ImVec2& mouse_pos, const ImVec2& keyframe_pos) const;
	int FindKeyframeUnderMouse(int track_index, const ImVec2& mouse_pos) const;

	// Keyframe operations
	void AddKeyframe(int track_index, float time);
	void DeleteKeyframe(int track_index, int keyframe_index);
	void SelectKeyframe(int track_index, int keyframe_index);
	void ClearSelection();

	// Track operations
	void AddTrack(const std::string& property_name);
	void RemoveTrack(int track_index);

	// Playback
	void Play();
	void Pause();
	void Stop();
	void UpdatePlayback(float delta_time);

	engine::animation::AnimationClip clip_;

	// Timeline state
	float timeline_zoom_ = 1.0f; // Pixels per frame
	float timeline_scroll_ = 0.0f; // Horizontal scroll offset
	float current_time_ = 0.0f; // Current playhead position
	float fps_ = 30.0f; // Frames per second

	// Playback state
	bool is_playing_ = false;
	bool is_paused_ = false;
	bool is_looping_ = false;

	// Selection state
	int selected_track_ = -1;
	int selected_keyframe_ = -1;
	int hovered_track_ = -1;
	int hovered_keyframe_ = -1;

	// Interaction state
	bool is_dragging_playhead_ = false;
	bool is_dragging_keyframe_ = false;
	float drag_start_time_ = 0.0f;

	// UI constants
	static constexpr float TIMELINE_HEIGHT = 40.0f;
	static constexpr float TRACK_HEIGHT = 30.0f;
	static constexpr float TRACK_LABEL_WIDTH = 150.0f;
	static constexpr float KEYFRAME_SIZE = 8.0f;
	static constexpr float MIN_ZOOM = 0.5f;
	static constexpr float MAX_ZOOM = 10.0f;
	static constexpr float ZOOM_SPEED = 0.1f;

	// Context menu state
	enum class ContextTarget { None, Timeline, Track, Keyframe };
	ContextTarget context_target_ = ContextTarget::None;
	int context_track_ = -1;
	int context_keyframe_ = -1;
	ImVec2 context_menu_pos_{0.0f, 0.0f};

	// Cached per-frame values
	ImVec2 timeline_canvas_p0_{0.0f, 0.0f};
	ImVec2 timeline_canvas_p1_{0.0f, 0.0f};
	ImVec2 tracks_canvas_p0_{0.0f, 0.0f};
	ImVec2 tracks_canvas_p1_{0.0f, 0.0f};

	// Input buffers
	char new_track_name_buffer_[256] = "";
	float keyframe_value_buffer_ = 0.0f;
	int interpolation_mode_buffer_ = 0;

	// Dialog state
	std::string current_file_path_;
	FileDialogPopup open_dialog_{"Open Animation", FileDialogMode::Open, {".json"}};
	FileDialogPopup save_dialog_{"Save Animation As", FileDialogMode::Save, {".json"}};
	bool show_add_track_dialog_ = false;
	bool show_keyframe_editor_ = false;
};

} // namespace editor
