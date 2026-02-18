# Audio API

Citrus Engine provides a cross-platform audio system powered by miniaudio. This document covers how to load and play sounds in your game.

## Overview

The audio system supports:

- **Audio playback**: WAV, OGG, and other common formats
- **3D spatial audio**: Position-based audio with distance attenuation
- **Volume and pitch control**: Per-sound control
- **Looping**: Seamless audio loops

## Audio System Initialization

The audio system is automatically initialized when you create the engine:

```cpp
import engine;

engine::Engine eng;
eng.Init(1280, 720);

// Audio system is ready to use
auto& audio = engine::audio::AudioSystem::Get();
```

## Core Concepts

### AudioClip

An **AudioClip** represents loaded audio data:

```cpp
struct AudioClip {
    std::string file_path;
    uint32_t id;
    // Internal audio data
};
```

### AudioSource

An **AudioSource** is an ECS component for 3D positioned audio:

```cpp
struct AudioSource {
    uint32_t clip_id{0};          // Which clip to play
    float volume{1.0f};            // 0.0 to 1.0
    float pitch{1.0f};             // Pitch multiplier
    bool looping{false};           // Loop the sound
    bool play_on_awake{false};     // Auto-play when entity is created
    bool is_3d{false};             // Enable 3D positioning
    float min_distance{1.0f};      // Distance where volume is at max
    float max_distance{100.0f};    // Distance where volume is at min
    float attenuation{1.0f};       // Rolloff factor
};
```

### AudioListener

An **AudioListener** represents the player's "ears" in the world:

```cpp
struct AudioListener {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 velocity{0.0f};  // For doppler effect (future)
};
```

## Loading Audio Clips

Load audio files using the `AudioSystem`:

```cpp
auto& audio = engine::audio::AudioSystem::Get();

// Load a clip
uint32_t jump_clip = audio.LoadClip("audio/jump.wav");
uint32_t music_clip = audio.LoadClip("audio/background_music.ogg");

// Check if loaded successfully
if (jump_clip == 0) {
    std::cerr << "Failed to load jump.wav" << std::endl;
}
```

**Supported formats:**
- WAV (uncompressed)
- OGG Vorbis
- MP3 (if enabled)
- FLAC (if enabled)

## Playing Sounds (Direct API)

### Simple Playback

Play sounds directly via the `AudioSystem`:

```cpp
auto& audio = engine::audio::AudioSystem::Get();

// Load clip
uint32_t clip_id = audio.LoadClip("audio/explosion.wav");

// Play sound (returns a handle for controlling playback)
uint32_t handle = audio.PlaySound(clip_id, 1.0f, false);

// Play looping background music
uint32_t music_handle = audio.PlaySound(music_clip, 0.5f, true);
```

### Controlling Playback

```cpp
// Adjust volume (0.0 to 1.0)
audio.SetVolume(handle, 0.8f);

// Adjust pitch (1.0 = normal, 2.0 = double speed, 0.5 = half speed)
audio.SetPitch(handle, 1.2f);

// Pause
audio.PauseSound(handle);

// Resume
audio.ResumeSound(handle);

// Stop
audio.StopSound(handle);
```

## 3D Spatial Audio

For positioned sounds in the game world, use the ECS components:

### Setting Up the Listener

Attach an `AudioListener` to your camera or player entity:

```cpp
auto camera = eng.ecs.CreateEntity("Camera");
camera.set<engine::components::Transform>({{0.0f, 0.0f, 0.0f}});
camera.set<engine::audio::AudioListener>({
    .position = {0.0f, 0.0f, 0.0f},
    .forward = {0.0f, 0.0f, -1.0f},
    .up = {0.0f, 1.0f, 0.0f}
});
```

!!! note
    Only one `AudioListener` should be active at a time (typically on the camera).

### Creating 3D Audio Sources

Attach an `AudioSource` component to entities that emit sound:

```cpp
// Create an entity with audio
auto enemy = eng.ecs.CreateEntity("Enemy");
enemy.set<engine::components::Transform>({{10.0f, 0.0f, 5.0f}});

// Load audio clip
uint32_t growl_clip = audio.LoadClip("audio/enemy_growl.wav");

// Attach audio source component
enemy.set<engine::audio::AudioSource>({
    .clip_id = growl_clip,
    .volume = 1.0f,
    .looping = true,
    .play_on_awake = true,     // Start playing immediately
    .is_3d = true,              // Enable 3D positioning
    .min_distance = 5.0f,       // Full volume within 5 units
    .max_distance = 50.0f,      // Silent beyond 50 units
    .attenuation = 1.0f         // Linear falloff
});
```

### Updating 3D Audio

The audio system automatically updates 3D sources each frame:

```cpp
// In your main loop
audio.Update(delta_time);

// This synchronizes:
// - AudioSource positions from Transform components
// - AudioListener position from Transform
// - Volume based on distance
```

### Distance Attenuation

The volume of 3D sounds decreases with distance:

```
volume = max(0, (max_distance - distance) / (max_distance - min_distance))^attenuation
```

- **min_distance**: Distance where volume is at maximum
- **max_distance**: Distance where volume reaches zero
- **attenuation**: Rolloff exponent (1.0 = linear, 2.0 = quadratic)

**Example:**

```cpp
// Loud explosion with long falloff
explosion_source.set<engine::audio::AudioSource>({
    .is_3d = true,
    .min_distance = 10.0f,   // Loud within 10 units
    .max_distance = 200.0f,  // Audible up to 200 units
    .attenuation = 2.0f      // Quadratic falloff (realistic)
});

// Quiet ambient sound
ambient_source.set<engine::audio::AudioSource>({
    .is_3d = true,
    .min_distance = 1.0f,
    .max_distance = 20.0f,
    .attenuation = 1.0f      // Linear falloff
});
```

## Complete Example

Here's a complete example setting up audio in a game scene:

```cpp
import engine;

void SetupAudioScene(engine::Engine& eng) {
    auto& audio = engine::audio::AudioSystem::Get();
    
    // Load audio clips
    uint32_t music_clip = audio.LoadClip("audio/background_music.ogg");
    uint32_t footstep_clip = audio.LoadClip("audio/footstep.wav");
    uint32_t ambient_clip = audio.LoadClip("audio/wind.wav");
    
    // Create camera with audio listener
    auto camera = eng.ecs.CreateEntity("Camera");
    camera.set<engine::components::Transform>({{0.0f, 1.6f, 0.0f}});
    camera.set<engine::audio::AudioListener>({
        .position = {0.0f, 1.6f, 0.0f},
        .forward = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f}
    });
    
    // Play background music (non-3D, global)
    uint32_t music_handle = audio.PlaySound(music_clip, 0.3f, true);
    
    // Create ambient sound source (3D positioned)
    auto ambient_source = eng.ecs.CreateEntity("WindSource");
    ambient_source.set<engine::components::Transform>({{20.0f, 0.0f, 0.0f}});
    ambient_source.set<engine::audio::AudioSource>({
        .clip_id = ambient_clip,
        .volume = 0.5f,
        .looping = true,
        .play_on_awake = true,
        .is_3d = true,
        .min_distance = 5.0f,
        .max_distance = 50.0f
    });
    
    // Create player (footsteps triggered manually)
    auto player = eng.ecs.CreateEntity("Player");
    player.set<engine::components::Transform>({{0.0f, 0.0f, 0.0f}});
}

void UpdatePlayer(engine::Engine& eng, float delta_time) {
    auto& audio = engine::audio::AudioSystem::Get();
    static uint32_t footstep_clip = audio.LoadClip("audio/footstep.wav");
    static float footstep_timer = 0.0f;
    
    // Get player
    auto player = eng.ecs.GetWorld().lookup("Player");
    if (!player) return;
    
    // Check if moving
    glm::vec3 movement{0.0f};
    if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::W)) {
        movement.z -= 1.0f;
    }
    if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::S)) {
        movement.z += 1.0f;
    }
    
    // Play footsteps while moving
    if (glm::length(movement) > 0.0f) {
        footstep_timer += delta_time;
        if (footstep_timer >= 0.5f) {  // Every 0.5 seconds
            audio.PlaySound(footstep_clip, 1.0f, false);
            footstep_timer = 0.0f;
        }
    } else {
        footstep_timer = 0.0f;
    }
}
```

## Audio Patterns

### One-Shot Sound Effects

For sounds that play once (UI clicks, gunshots):

```cpp
uint32_t click_clip = audio.LoadClip("audio/click.wav");

// In button click handler:
audio.PlaySound(click_clip, 1.0f, false);
```

### Background Music

For looping background music:

```cpp
uint32_t music_clip = audio.LoadClip("audio/level_theme.ogg");
uint32_t music_handle = audio.PlaySound(music_clip, 0.4f, true);

// Fade out on level end
for (float vol = 0.4f; vol >= 0.0f; vol -= 0.01f) {
    audio.SetVolume(music_handle, vol);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
audio.StopSound(music_handle);
```

### Positioned Ambient Loops

For environmental sounds (waterfalls, fires):

```cpp
auto campfire = eng.ecs.CreateEntity("Campfire");
campfire.set<engine::components::Transform>({{25.0f, 0.0f, 10.0f}});
campfire.set<engine::audio::AudioSource>({
    .clip_id = audio.LoadClip("audio/fire.wav"),
    .volume = 0.8f,
    .looping = true,
    .play_on_awake = true,
    .is_3d = true,
    .min_distance = 3.0f,
    .max_distance = 30.0f,
    .attenuation = 1.5f
});
```

### Dynamic Volume Based on Game State

```cpp
void UpdateMusicVolume(engine::Engine& eng, uint32_t music_handle) {
    auto& audio = engine::audio::AudioSystem::Get();
    
    // Get player health (example)
    auto player = eng.ecs.GetWorld().lookup("Player");
    auto* health = player.get<HealthComponent>();
    
    if (health) {
        // Reduce music volume when low on health
        float volume = health->current_health / health->max_health;
        audio.SetVolume(music_handle, volume * 0.4f);
    }
}
```

## Audio Groups / Categories

For managing groups of sounds (e.g., mute all sound effects):

```cpp
struct AudioCategory {
    std::vector<uint32_t> handles;
    float volume_multiplier{1.0f};
};

std::unordered_map<std::string, AudioCategory> audio_categories;

// Add sound to category
void PlayInCategory(const std::string& category, uint32_t clip_id) {
    auto& audio = engine::audio::AudioSystem::Get();
    uint32_t handle = audio.PlaySound(clip_id, 1.0f, false);
    audio_categories[category].handles.push_back(handle);
}

// Adjust category volume
void SetCategoryVolume(const std::string& category, float volume) {
    auto& cat = audio_categories[category];
    cat.volume_multiplier = volume;
    
    for (uint32_t handle : cat.handles) {
        engine::audio::AudioSystem::Get().SetVolume(handle, volume);
    }
}

// Usage:
PlayInCategory("SFX", explosion_clip);
SetCategoryVolume("SFX", 0.0f);  // Mute all sound effects
```

## Best Practices

1. **Preload frequently used clips**: Load at scene start, not during gameplay
2. **Use looping for continuous sounds**: Music, ambient sounds
3. **Limit simultaneous sounds**: Too many sounds can cause distortion
4. **Normalize audio files**: Keep consistent volume levels in source files
5. **Use appropriate formats**: OGG for music (smaller), WAV for short effects
6. **Set min/max distance carefully**: Balance realism vs. gameplay audibility
7. **Update listener position**: Sync with camera transform each frame

## Performance Tips

- **Pool sound handles**: Reuse handles instead of creating new ones
- **Limit 3D sources**: Only nearby sources need 3D positioning
- **Use compressed formats**: OGG instead of WAV for large files
- **Stop unused sounds**: Don't let finished sounds accumulate

## Troubleshooting

**No sound playing:**
- Check audio file path is correct
- Verify clip loaded successfully (`clip_id != 0`)
- Ensure volume > 0.0
- Check system audio isn't muted

**3D audio not working:**
- Verify `is_3d = true` on AudioSource
- Check AudioListener exists and is positioned correctly
- Ensure entity has Transform component
- Verify distance settings (min_distance < max_distance)

**Crackling/distortion:**
- Too many sounds playing simultaneously
- Audio files have clipping in source
- System audio buffer size too small

## Future Features

Planned audio features:

- **Audio mixer**: Per-category volume control
- **Audio effects**: Reverb, echo, filters
- **Doppler effect**: Pitch shifting based on velocity
- **Occlusion**: Volume reduction through walls
- **Audio streaming**: For very large files

## Further Reading

- **[Getting Started](getting-started.md)** - Set up your project
- **[Architecture Overview](architecture.md)** - How audio integrates with ECS
- **[Scene Management](scenes.md)** - Save/load audio configurations
