#include <gtest/gtest.h>

import engine.audio.system;

using namespace engine::audio;

class AudioSystemTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Shutdown any leftover state from previous tests (singleton)
		AudioSystem::Get().Shutdown();
	}

	void TearDown() override {
		AudioSystem::Get().Shutdown();
	}
};

TEST_F(AudioSystemTest, singleton_get_returns_same_instance) {
	auto& a = AudioSystem::Get();
	auto& b = AudioSystem::Get();
	EXPECT_EQ(&a, &b);
}

TEST_F(AudioSystemTest, not_initialized_by_default) {
	EXPECT_FALSE(AudioSystem::Get().IsInitialized());
}

TEST_F(AudioSystemTest, initialize_and_shutdown_lifecycle) {
	auto& audio = AudioSystem::Get();
	bool ok = audio.Initialize();
	// Audio init may fail in headless CI environments; either outcome is valid
	if (ok) {
		EXPECT_TRUE(audio.IsInitialized());
		audio.Shutdown();
		EXPECT_FALSE(audio.IsInitialized());
	} else {
		EXPECT_FALSE(audio.IsInitialized());
	}
}

TEST_F(AudioSystemTest, double_initialize_is_safe) {
	auto& audio = AudioSystem::Get();
	bool first = audio.Initialize();
	if (first) {
		bool second = audio.Initialize();
		EXPECT_TRUE(second);
		EXPECT_TRUE(audio.IsInitialized());
	}
}

TEST_F(AudioSystemTest, load_clip_invalid_path_returns_zero) {
	auto& audio = AudioSystem::Get();
	if (!audio.Initialize()) {
		GTEST_SKIP() << "Audio init unavailable in this environment";
	}
	uint32_t clip_id = audio.LoadClip("nonexistent_file_that_does_not_exist.wav");
	EXPECT_EQ(clip_id, 0u);
}

TEST_F(AudioSystemTest, play_sound_invalid_clip_returns_zero) {
	auto& audio = AudioSystem::Get();
	if (!audio.Initialize()) {
		GTEST_SKIP() << "Audio init unavailable in this environment";
	}
	uint32_t handle = audio.PlaySoundClip(9999);
	EXPECT_EQ(handle, 0u);
}

TEST_F(AudioSystemTest, operations_without_init_do_not_crash) {
	auto& audio = AudioSystem::Get();
	// None of these should crash when system is not initialized
	EXPECT_EQ(audio.LoadClip("test.wav"), 0u);
	EXPECT_EQ(audio.PlaySoundClip(1), 0u);
	audio.StopSound(1);
	audio.PauseSound(1);
	audio.ResumeSound(1);
	audio.SetVolume(1, 0.5f);
	audio.SetPitch(1, 1.0f);
	audio.SetSourcePosition(1, 0.0f, 0.0f, 0.0f);
	audio.Update(0.016f);
}

TEST_F(AudioSystemTest, get_clip_returns_null_for_unknown_id) {
	auto& audio = AudioSystem::Get();
	if (!audio.Initialize()) {
		GTEST_SKIP() << "Audio init unavailable in this environment";
	}
	EXPECT_EQ(audio.GetClip(9999), nullptr);
}
