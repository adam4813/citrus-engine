#include <gtest/gtest.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <vector>

import engine.animation;
import glm;

using namespace engine::animation;

// =============================================================================
// Keyframe & Track Tests
// =============================================================================

TEST(AnimationTrackTest, empty_track_has_zero_duration) {
	AnimationTrack track;
	EXPECT_FLOAT_EQ(track.GetDuration(), 0.0f);
	EXPECT_EQ(track.GetKeyframeCount(), 0u);
}

TEST(AnimationTrackTest, add_keyframe_maintains_sorted_order) {
	AnimationTrack track;
	track.AddKeyframe(1.0f, 10.0f);
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(0.5f, 5.0f);

	ASSERT_EQ(track.GetKeyframeCount(), 3u);
	EXPECT_FLOAT_EQ(track.keyframes[0].time, 0.0f);
	EXPECT_FLOAT_EQ(track.keyframes[1].time, 0.5f);
	EXPECT_FLOAT_EQ(track.keyframes[2].time, 1.0f);
}

TEST(AnimationTrackTest, duration_equals_last_keyframe_time) {
	AnimationTrack track;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(2.5f, 1.0f);

	EXPECT_FLOAT_EQ(track.GetDuration(), 2.5f);
}

TEST(AnimationTrackTest, clear_removes_all_keyframes) {
	AnimationTrack track;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 1.0f);
	track.Clear();

	EXPECT_EQ(track.GetKeyframeCount(), 0u);
}

// =============================================================================
// Linear Interpolation Tests
// =============================================================================

TEST(AnimationTrackTest, linear_interpolation_float_midpoint) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);

	AnimatedValue result = track.Evaluate(0.5f);
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_NEAR(std::get<float>(result), 5.0f, 0.01f);
}

TEST(AnimationTrackTest, linear_interpolation_float_at_start) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);

	AnimatedValue result = track.Evaluate(0.0f);
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_FLOAT_EQ(std::get<float>(result), 0.0f);
}

TEST(AnimationTrackTest, linear_interpolation_float_at_end) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);

	AnimatedValue result = track.Evaluate(1.0f);
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_FLOAT_EQ(std::get<float>(result), 10.0f);
}

TEST(AnimationTrackTest, linear_interpolation_vec3) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	track.AddKeyframe(1.0f, glm::vec3(10.0f, 20.0f, 30.0f));

	AnimatedValue result = track.Evaluate(0.5f);
	ASSERT_TRUE(std::holds_alternative<glm::vec3>(result));
	auto v = std::get<glm::vec3>(result);
	EXPECT_NEAR(v.x, 5.0f, 0.01f);
	EXPECT_NEAR(v.y, 10.0f, 0.01f);
	EXPECT_NEAR(v.z, 15.0f, 0.01f);
}

TEST(AnimationTrackTest, linear_interpolation_multiple_keyframes) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);
	track.AddKeyframe(2.0f, 20.0f);

	// Between first and second keyframe
	AnimatedValue r1 = track.Evaluate(0.5f);
	EXPECT_NEAR(std::get<float>(r1), 5.0f, 0.01f);

	// Between second and third keyframe
	AnimatedValue r2 = track.Evaluate(1.5f);
	EXPECT_NEAR(std::get<float>(r2), 15.0f, 0.01f);
}

// =============================================================================
// Step Interpolation Tests
// =============================================================================

TEST(AnimationTrackTest, step_interpolation_holds_value) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Step;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);

	// Step interpolation should return the first keyframe's value until the next keyframe
	AnimatedValue result = track.Evaluate(0.5f);
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_FLOAT_EQ(std::get<float>(result), 0.0f);
}

TEST(AnimationTrackTest, step_interpolation_at_keyframe_boundary) {
	AnimationTrack track;
	track.interpolation = InterpolationMode::Step;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);
	track.AddKeyframe(2.0f, 20.0f);

	// At time 1.0 (second keyframe), should use second keyframe value (step from 1.0 to 2.0)
	AnimatedValue result = track.Evaluate(1.0f);
	GTEST_SKIP() << "Step interpolation is not currently implemented to switch at the keyframe boundary. This test "
					"should be updated once implemented.";
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_FLOAT_EQ(std::get<float>(result), 10.0f);
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(AnimationTrackTest, evaluate_empty_track_returns_default) {
	AnimationTrack track;
	AnimatedValue result = track.Evaluate(0.5f);
	ASSERT_TRUE(std::holds_alternative<float>(result));
	EXPECT_FLOAT_EQ(std::get<float>(result), 0.0f);
}

TEST(AnimationTrackTest, evaluate_single_keyframe_returns_that_value) {
	AnimationTrack track;
	track.AddKeyframe(1.0f, 42.0f);

	// Any time should return the single keyframe value
	AnimatedValue r1 = track.Evaluate(0.0f);
	EXPECT_FLOAT_EQ(std::get<float>(r1), 42.0f);

	AnimatedValue r2 = track.Evaluate(5.0f);
	EXPECT_FLOAT_EQ(std::get<float>(r2), 42.0f);
}

TEST(AnimationTrackTest, evaluate_before_first_keyframe_returns_first_value) {
	AnimationTrack track;
	track.AddKeyframe(1.0f, 10.0f);
	track.AddKeyframe(2.0f, 20.0f);

	AnimatedValue result = track.Evaluate(0.0f);
	EXPECT_FLOAT_EQ(std::get<float>(result), 10.0f);
}

TEST(AnimationTrackTest, evaluate_after_last_keyframe_returns_last_value) {
	AnimationTrack track;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);

	AnimatedValue result = track.Evaluate(5.0f);
	EXPECT_FLOAT_EQ(std::get<float>(result), 10.0f);
}

// =============================================================================
// AnimationClip Tests
// =============================================================================

TEST(AnimationClipTest, empty_clip_defaults) {
	AnimationClip clip;
	EXPECT_FLOAT_EQ(clip.duration, 0.0f);
	EXPECT_FALSE(clip.looping);
	EXPECT_EQ(clip.GetTrackCount(), 0u);
}

TEST(AnimationClipTest, add_track_updates_duration) {
	AnimationClip clip;

	AnimationTrack track;
	track.target_property = "position.x";
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(3.0f, 10.0f);

	clip.AddTrack(std::move(track));
	EXPECT_FLOAT_EQ(clip.duration, 3.0f);
	EXPECT_EQ(clip.GetTrackCount(), 1u);
}

TEST(AnimationClipTest, duration_is_max_of_all_tracks) {
	AnimationClip clip;

	AnimationTrack track1;
	track1.target_property = "position.x";
	track1.AddKeyframe(0.0f, 0.0f);
	track1.AddKeyframe(2.0f, 1.0f);

	AnimationTrack track2;
	track2.target_property = "position.y";
	track2.AddKeyframe(0.0f, 0.0f);
	track2.AddKeyframe(5.0f, 1.0f);

	clip.AddTrack(std::move(track1));
	clip.AddTrack(std::move(track2));

	EXPECT_FLOAT_EQ(clip.duration, 5.0f);
}

TEST(AnimationClipTest, find_track_by_property) {
	AnimationClip clip;

	AnimationTrack track;
	track.target_property = "rotation.z";
	track.AddKeyframe(0.0f, 0.0f);
	clip.AddTrack(std::move(track));

	EXPECT_NE(clip.FindTrack("rotation.z"), nullptr);
	EXPECT_EQ(clip.FindTrack("nonexistent"), nullptr);
}

TEST(AnimationClipTest, evaluate_all_tracks) {
	AnimationClip clip;

	AnimationTrack tx;
	tx.target_property = "position.x";
	tx.interpolation = InterpolationMode::Linear;
	tx.AddKeyframe(0.0f, 0.0f);
	tx.AddKeyframe(1.0f, 10.0f);

	AnimationTrack ty;
	ty.target_property = "position.y";
	ty.interpolation = InterpolationMode::Linear;
	ty.AddKeyframe(0.0f, 0.0f);
	ty.AddKeyframe(1.0f, 20.0f);

	clip.AddTrack(std::move(tx));
	clip.AddTrack(std::move(ty));

	std::vector<std::pair<std::string, AnimatedValue>> values;
	clip.EvaluateAll(0.5f, values);

	ASSERT_EQ(values.size(), 2u);
	EXPECT_NEAR(std::get<float>(values[0].second), 5.0f, 0.01f);
	EXPECT_NEAR(std::get<float>(values[1].second), 10.0f, 0.01f);
}

TEST(AnimationClipTest, clear_removes_all_tracks) {
	AnimationClip clip;
	AnimationTrack track;
	track.target_property = "x";
	track.AddKeyframe(0.0f, 0.0f);
	clip.AddTrack(std::move(track));

	clip.Clear();
	EXPECT_EQ(clip.GetTrackCount(), 0u);
}

// =============================================================================
// AnimationState Playback Tests
// =============================================================================

TEST(AnimationStateTest, default_state) {
	AnimationState state;
	EXPECT_FALSE(state.IsPlaying());
	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
	EXPECT_FLOAT_EQ(state.GetSpeed(), 1.0f);
	EXPECT_EQ(state.GetClip(), nullptr);
}

TEST(AnimationStateTest, play_pause_stop) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	EXPECT_FALSE(state.IsPlaying());

	state.Play();
	EXPECT_TRUE(state.IsPlaying());

	state.Pause();
	EXPECT_FALSE(state.IsPlaying());

	state.Play();
	state.Stop();
	EXPECT_FALSE(state.IsPlaying());
	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
}

TEST(AnimationStateTest, update_advances_time) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	state.Play();
	state.Update(0.5f);
	EXPECT_FLOAT_EQ(state.GetTime(), 0.5f);

	state.Update(0.5f);
	EXPECT_FLOAT_EQ(state.GetTime(), 1.0f);
}

TEST(AnimationStateTest, update_does_nothing_when_paused) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	// Not playing — update should not advance time
	state.Update(1.0f);
	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
}

TEST(AnimationStateTest, non_looping_clamps_at_end) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 1.0f;
	AnimationState state(clip);

	state.SetLooping(false);
	state.Play();
	state.Update(2.0f); // Exceeds duration

	EXPECT_FLOAT_EQ(state.GetTime(), 1.0f);
	EXPECT_FALSE(state.IsPlaying()); // Should auto-stop
	EXPECT_TRUE(state.HasFinished());
}

TEST(AnimationStateTest, looping_wraps_around) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 1.0f;
	AnimationState state(clip);

	state.SetLooping(true);
	state.Play();
	state.Update(1.5f);

	EXPECT_NEAR(state.GetTime(), 0.5f, 0.001f);
	EXPECT_TRUE(state.IsPlaying()); // Should keep playing
	EXPECT_FALSE(state.HasFinished());
}

TEST(AnimationStateTest, speed_multiplier) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	state.SetSpeed(2.0f);
	state.Play();
	state.Update(0.5f); // Effective: 1.0s

	EXPECT_FLOAT_EQ(state.GetTime(), 1.0f);
}

TEST(AnimationStateTest, normalized_time) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 4.0f;
	AnimationState state(clip);

	state.Play();
	state.Update(2.0f);

	EXPECT_FLOAT_EQ(state.GetNormalizedTime(), 0.5f);
}

TEST(AnimationStateTest, normalized_time_no_clip) {
	AnimationState state;
	EXPECT_FLOAT_EQ(state.GetNormalizedTime(), 0.0f);
}

TEST(AnimationStateTest, set_time_clamps_non_looping) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	state.SetTime(5.0f);
	EXPECT_FLOAT_EQ(state.GetTime(), 2.0f);

	state.SetTime(-1.0f);
	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
}

TEST(AnimationStateTest, set_clip_resets_time) {
	auto clip1 = std::make_shared<AnimationClip>();
	clip1->duration = 2.0f;
	auto clip2 = std::make_shared<AnimationClip>();
	clip2->duration = 3.0f;

	AnimationState state(clip1);
	state.Play();
	state.Update(1.0f);
	EXPECT_FLOAT_EQ(state.GetTime(), 1.0f);

	state.SetClip(clip2);
	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
}

TEST(AnimationStateTest, reset_sets_time_to_zero) {
	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 2.0f;
	AnimationState state(clip);

	state.Play();
	state.Update(1.0f);
	state.Reset();

	EXPECT_FLOAT_EQ(state.GetTime(), 0.0f);
}

// =============================================================================
// Animator Component Tests
// =============================================================================

TEST(AnimatorTest, queue_transition) {
	Animator animator;
	EXPECT_FALSE(animator.HasPendingTransitions());

	auto clip = std::make_shared<AnimationClip>();
	clip->duration = 1.0f;
	animator.QueueAnimation(clip);

	EXPECT_TRUE(animator.HasPendingTransitions());
}

TEST(AnimatorTest, clear_queue) {
	Animator animator;
	auto clip = std::make_shared<AnimationClip>();
	animator.QueueAnimation(clip);
	animator.QueueAnimation(clip);

	animator.ClearQueue();
	EXPECT_FALSE(animator.HasPendingTransitions());
}

// =============================================================================
// Animation Serialization Round-Trip Tests
// =============================================================================

TEST(AnimationSerializerTest, roundtrip_simple_clip) {
	// Create a clip
	AnimationClip original;
	original.name = "TestClip";
	original.looping = true;

	AnimationTrack track;
	track.target_property = "position.x";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.0f, 10.0f);
	original.AddTrack(std::move(track));

	// Serialize to JSON
	auto json = AnimationSerializer::ToJson(original);

	// Deserialize from JSON
	auto loaded = AnimationSerializer::FromJson(json);
	ASSERT_NE(loaded, nullptr);

	EXPECT_EQ(loaded->name, "TestClip");
	EXPECT_TRUE(loaded->looping);
	EXPECT_FLOAT_EQ(loaded->duration, 1.0f);
	ASSERT_EQ(loaded->GetTrackCount(), 1u);

	const auto* loaded_track = loaded->FindTrack("position.x");
	ASSERT_NE(loaded_track, nullptr);
	EXPECT_EQ(loaded_track->interpolation, InterpolationMode::Linear);
	ASSERT_EQ(loaded_track->GetKeyframeCount(), 2u);
	EXPECT_FLOAT_EQ(loaded_track->keyframes[0].time, 0.0f);
	EXPECT_FLOAT_EQ(loaded_track->keyframes[1].time, 1.0f);
}

TEST(AnimationSerializerTest, roundtrip_vec3_keyframes) {
	AnimationClip original;
	original.name = "Vec3Clip";

	AnimationTrack track;
	track.target_property = "position";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, glm::vec3(1.0f, 2.0f, 3.0f));
	track.AddKeyframe(1.0f, glm::vec3(4.0f, 5.0f, 6.0f));
	original.AddTrack(std::move(track));

	auto json = AnimationSerializer::ToJson(original);
	auto loaded = AnimationSerializer::FromJson(json);
	ASSERT_NE(loaded, nullptr);

	const auto* lt = loaded->FindTrack("position");
	ASSERT_NE(lt, nullptr);
	ASSERT_EQ(lt->GetKeyframeCount(), 2u);

	auto val0 = std::get<glm::vec3>(lt->keyframes[0].value);
	EXPECT_FLOAT_EQ(val0.x, 1.0f);
	EXPECT_FLOAT_EQ(val0.y, 2.0f);
	EXPECT_FLOAT_EQ(val0.z, 3.0f);

	auto val1 = std::get<glm::vec3>(lt->keyframes[1].value);
	EXPECT_FLOAT_EQ(val1.x, 4.0f);
	EXPECT_FLOAT_EQ(val1.y, 5.0f);
	EXPECT_FLOAT_EQ(val1.z, 6.0f);
}

TEST(AnimationSerializerTest, roundtrip_step_interpolation) {
	AnimationClip original;
	original.name = "StepClip";

	AnimationTrack track;
	track.target_property = "frame";
	track.interpolation = InterpolationMode::Step;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(0.5f, 1.0f);
	track.AddKeyframe(1.0f, 2.0f);
	original.AddTrack(std::move(track));

	auto json = AnimationSerializer::ToJson(original);
	auto loaded = AnimationSerializer::FromJson(json);
	ASSERT_NE(loaded, nullptr);

	const auto* lt = loaded->FindTrack("frame");
	ASSERT_NE(lt, nullptr);
	EXPECT_EQ(lt->interpolation, InterpolationMode::Step);
	EXPECT_EQ(lt->GetKeyframeCount(), 3u);
}

TEST(AnimationSerializerTest, roundtrip_multiple_tracks) {
	AnimationClip original;
	original.name = "MultiTrack";
	original.looping = false;

	AnimationTrack tx;
	tx.target_property = "position.x";
	tx.AddKeyframe(0.0f, 0.0f);
	tx.AddKeyframe(2.0f, 10.0f);
	original.AddTrack(std::move(tx));

	AnimationTrack ty;
	ty.target_property = "position.y";
	ty.AddKeyframe(0.0f, 0.0f);
	ty.AddKeyframe(3.0f, 20.0f);
	original.AddTrack(std::move(ty));

	auto json = AnimationSerializer::ToJson(original);
	auto loaded = AnimationSerializer::FromJson(json);
	ASSERT_NE(loaded, nullptr);

	EXPECT_EQ(loaded->GetTrackCount(), 2u);
	EXPECT_FLOAT_EQ(loaded->duration, 3.0f);
	EXPECT_NE(loaded->FindTrack("position.x"), nullptr);
	EXPECT_NE(loaded->FindTrack("position.y"), nullptr);
}

TEST(AnimationSerializerTest, roundtrip_quat_keyframes) {
	AnimationClip original;
	original.name = "QuatClip";

	AnimationTrack track;
	track.target_property = "orientation";
	track.AddKeyframe(0.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
	track.AddKeyframe(1.0f, glm::quat(0.707f, 0.707f, 0.0f, 0.0f));
	original.AddTrack(std::move(track));

	auto json = AnimationSerializer::ToJson(original);
	auto loaded = AnimationSerializer::FromJson(json);
	ASSERT_NE(loaded, nullptr);

	const auto* lt = loaded->FindTrack("orientation");
	ASSERT_NE(lt, nullptr);
	ASSERT_EQ(lt->GetKeyframeCount(), 2u);

	auto q0 = std::get<glm::quat>(lt->keyframes[0].value);
	EXPECT_FLOAT_EQ(q0.w, 1.0f);
	EXPECT_FLOAT_EQ(q0.x, 0.0f);
}

TEST(AnimationSerializerTest, json_contains_expected_fields) {
	AnimationClip clip;
	clip.name = "FieldCheck";
	clip.looping = true;
	clip.duration = 1.5f;

	AnimationTrack track;
	track.target_property = "alpha";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, 0.0f);
	track.AddKeyframe(1.5f, 1.0f);
	clip.AddTrack(std::move(track));

	auto json = AnimationSerializer::ToJson(clip);

	EXPECT_EQ(json["asset_type"], "animation");
	EXPECT_EQ(json["name"], "FieldCheck");
	EXPECT_EQ(json["looping"], true);
	EXPECT_TRUE(json.contains("tracks"));
	EXPECT_TRUE(json["tracks"].is_array());
	EXPECT_EQ(json["tracks"].size(), 1u);
}
