module;

#include <flecs.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

module engine.asset_registry;

import engine.audio;
import engine.ecs.component_registry;

namespace engine::assets {

void SoundAssetInfo::DoInitialize() {
	// No-op: clip is loaded in DoLoad
}

bool SoundAssetInfo::DoLoad() {
	if (file_path.empty()) {
		return true;
	}
	auto& audio_sys = audio::AudioSystem::Get();
	if (!audio_sys.IsInitialized()) {
		std::cerr << "SoundAssetInfo: AudioSystem not initialized, cannot load '" << name << "'" << '\n';
		return false;
	}
	// Check if already loaded by name
	clip_id = audio_sys.FindClipByName(name);
	if (clip_id != 0) {
		return true;
	}
	clip_id = audio_sys.LoadClipNamed(name, file_path);
	if (clip_id == 0) {
		std::cerr << "SoundAssetInfo: Failed to load audio clip '" << name << "' from " << file_path << '\n';
		return false;
	}
	std::cout << "SoundAssetInfo: Loaded audio clip '" << name << "' (clip_id=" << clip_id << ")" << '\n';
	return true;
}

void SoundAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	volume = j.value("volume", 1.0f);
	loop = j.value("loop", false);
	AssetInfo::FromJson(j);
}

void SoundAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	j["volume"] = volume;
	j["loop"] = loop;
	AssetInfo::ToJson(j);
}

void SoundAssetInfo::RegisterType() {
	AssetTypeRegistry::Instance()
			.RegisterType<SoundAssetInfo>(SoundAssetInfo::TYPE_NAME, AssetType::SOUND)
			.DisplayName("Sound")
			.Category("Audio")
			.Field("name", &SoundAssetInfo::name, "Name")
			.Field("file_path", &SoundAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Field("volume", &SoundAssetInfo::volume, "Volume")
			.Field("loop", &SoundAssetInfo::loop, "Loop")
			.Build();

	// Register file importers for raw audio files
	AssetCache::Instance().RegisterFileImporter({".wav", ".mp3", ".ogg", ".flac"},
			[](const std::string& name, const std::string& path) -> std::shared_ptr<AssetInfo> {
				auto asset = std::make_shared<SoundAssetInfo>(name);
				asset->file_path = path;
				return asset;
			});
}

void SoundAssetInfo::SetupRefBinding(flecs::world& world) {
	SetupRefBindingImpl<SoundAssetInfo, SoundRef, audio::AudioSource>(
			world,
			"SoundRef",
			"Audio",
			"SoundRefResolve",
			SoundAssetInfo::TYPE_NAME,
			[](const auto& asset, auto& target) { target.clip_id = asset->clip_id; },
			[](auto& target) { target.clip_id = 0; });
}

} // namespace engine::assets
