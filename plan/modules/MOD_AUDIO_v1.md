# MOD_AUDIO_v1

> **3D Spatial Audio System - Foundation Module**

## Executive Summary

The `engine.audio` module provides a comprehensive 3D spatial audio system built on OpenAL/Web Audio APIs for
cross-platform compatibility, delivering immersive soundscapes for the Colony Game Engine's simulation environment. This
module implements efficient audio streaming, dynamic range management, environmental audio effects, and real-time 3D
positioning to enhance the player experience with realistic audio feedback for colony activities, environmental
ambience, and interactive sound effects across desktop and WebAssembly platforms.

## Scope and Objectives

### In Scope

- [ ] 3D spatial audio with distance attenuation and doppler effects
- [ ] Multi-format audio streaming (OGG, WAV, MP3) with compression
- [ ] Environmental audio effects and reverb zones
- [ ] Audio source pooling and dynamic voice management
- [ ] Cross-platform audio backend abstraction (OpenAL/Web Audio)
- [ ] Real-time audio parameter modulation and mixing

### Out of Scope

- [ ] Advanced audio DSP effects and filters
- [ ] MIDI playback and synthesis
- [ ] Voice chat and network audio streaming
- [ ] Audio content authoring tools

### Primary Objectives

1. **Low Latency**: Audio response time under 10ms for interactive sounds
2. **Spatial Accuracy**: Precise 3D positioning with sub-meter accuracy
3. **Performance**: Support 64+ concurrent audio sources without frame drops

### Secondary Objectives

- Memory usage under 128MB for full audio content
- Seamless audio streaming without interruption
- Hot-reload audio assets during development

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages audio source components for entities producing sound
- **Entity Queries**: Queries entities with Transform, AudioSource, and AudioListener components
- **Component Dependencies**: Requires Transform for 3D positioning; optional AudioEmitter for environmental effects

#### Component Design

```cpp
// Audio-related components for ECS integration
struct AudioSource {
    AssetId audio_clip{0};
    float volume{1.0f};
    float pitch{1.0f};
    bool is_looping{false};
    bool is_3d{true};
    AudioSourceState state{AudioSourceState::Stopped};
    std::uint32_t source_id{0}; // OpenAL source ID
};

struct AudioListener {
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float master_volume{1.0f};
    bool is_active{true};
};

struct AudioEmitter {
    float min_distance{1.0f};
    float max_distance{100.0f};
    float rolloff_factor{1.0f};
    AudioAttenuationModel attenuation{AudioAttenuationModel::InverseDistance};
    std::optional<AssetId> reverb_zone;
};

// Component traits for audio
template<>();
struct ComponentTraits<AudioSource> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;
};

template<>();
struct ComponentTraits<AudioListener> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 4; // Multiple listeners for split-screen
};
```

#### System Integration

- **System Dependencies**: Runs after transform updates, before rendering
- **Component Access Patterns**: Read-only access to Transform; read-write to AudioSource states
- **Inter-System Communication**: Receives events from physics for collision sounds

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Audio batch - executes on dedicated audio thread for low latency
- **Thread Safety**: Audio API calls isolated to audio thread; component updates are lock-free
- **Data Dependencies**: Reads Transform data; writes to audio device buffers

#### Parallel Execution Strategy

```cpp
// Multi-threaded audio processing with real-time constraints
class AudioSystem : public ISystem {
public:
    // Main thread component updates
    void UpdateAudioComponents(const ComponentManager& components,
                              std::span<EntityId> entities,
                              const ThreadContext& context) override;
    
    // Audio thread processing with real-time priority
    void ProcessAudioThread(const ThreadContext& context);
    
    // Lock-free audio command submission
    void SubmitAudioCommand(const AudioCommand& command);

private:
    struct AudioCommand {
        enum Type { Play, Stop, SetVolume, SetPosition, SetPitch };
        Type type;
        std::uint32_t source_id;
        union {
            Vec3 position;
            float volume;
            float pitch;
            AssetId clip_id;
        } data;
    };
    
    // Lock-free command queue for audio thread
    moodycamel::ConcurrentQueue<AudioCommand> command_queue_;
    std::atomic<bool> audio_thread_running_{false};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Audio source data laid out for efficient batch processing
- **Memory Ordering**: Audio command queue uses sequentially consistent operations
- **Lock-Free Sections**: All audio parameter updates use lock-free command submission

### Public APIs

#### Primary Interface: `AudioSystemInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>

namespace engine::audio {

template<typename T>
concept AudioParameter = requires(T t) {
    requires std::is_arithmetic_v<T>;
    { t >= T{0} } -> std::convertible_to<bool>;
};

class AudioSystemInterface {
public:
    [[nodiscard]] auto Initialize(const AudioConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Audio playback control
    [[nodiscard]] auto PlaySound(AssetId clip_id, const Vec3& position, 
                                float volume = 1.0f) -> std::optional<AudioSourceId>;
    [[nodiscard]] auto PlayMusic(AssetId clip_id, float volume = 1.0f, 
                                bool loop = true) -> std::optional<AudioSourceId>;
    
    void StopSound(AudioSourceId source_id) noexcept;
    void StopAllSounds() noexcept;
    
    // 3D audio parameters
    void SetSourcePosition(AudioSourceId source_id, const Vec3& position) noexcept;
    void SetSourceVelocity(AudioSourceId source_id, const Vec3& velocity) noexcept;
    void SetListenerTransform(const Vec3& position, const Vec3& forward, const Vec3& up) noexcept;
    
    // Audio parameters
    template<AudioParameter T>
    void SetSourceVolume(AudioSourceId source_id, T volume) noexcept;
    
    template<AudioParameter T>
    void SetSourcePitch(AudioSourceId source_id, T pitch) noexcept;
    
    void SetMasterVolume(float volume) noexcept;
    
    // Environmental audio
    void SetReverbZone(const BoundingBox& area, const ReverbParameters& params) noexcept;
    void ClearReverbZone(const BoundingBox& area) noexcept;
    
    // Performance queries
    [[nodiscard]] auto GetActiveSources() const noexcept -> std::uint32_t;
    [[nodiscard]] auto GetAudioStats() const noexcept -> AudioStatistics;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<AudioSystemImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::audio
```

#### Scripting Interface Requirements

```cpp
// Audio scripting interface for dynamic sound control
class AudioScriptInterface {
public:
    // Type-safe audio playback from scripts
    [[nodiscard]] auto PlaySound(std::string_view clip_name, float x, float y, float z) -> std::optional<std::uint32_t>;
    [[nodiscard]] auto PlayMusic(std::string_view clip_name, float volume = 1.0f) -> bool;
    
    // Audio parameter control
    void SetSoundVolume(std::uint32_t source_id, float volume);
    void SetSoundPitch(std::uint32_t source_id, float pitch);
    void StopSound(std::uint32_t source_id);
    
    // Global audio settings
    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    void SetEffectsVolume(float volume);
    
    // Environmental audio
    void SetReverbEffect(std::string_view effect_name, float intensity);
    
    // Performance monitoring
    [[nodiscard]] auto GetActiveSoundCount() const -> std::uint32_t;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<AudioSystemInterface> audio_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **3D Audio**: Accurate spatial positioning with distance attenuation and stereo panning
- [ ] **Multi-Format Support**: Play OGG, WAV, and MP3 audio files across all platforms
- [ ] **Environmental Effects**: Apply reverb and environmental audio effects based on location

### Performance Requirements

- [ ] **Latency**: Audio response time under 10ms for interactive sound effects
- [ ] **Concurrency**: Support 64+ simultaneous audio sources without performance degradation
- [ ] **Memory**: Total audio memory usage under 128MB for full game content
- [ ] **CPU**: Audio processing under 5% CPU usage on target hardware

### Quality Requirements

- [ ] **Reliability**: Zero audio dropouts or glitches during normal operation
- [ ] **Maintainability**: All audio code covered by automated tests
- [ ] **Testability**: Headless audio testing with virtual audio devices
- [ ] **Documentation**: Complete audio implementation guide with parameter references

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(AudioTest, LatencyRequirement) {
    auto audio_system = engine::audio::AudioSystem{};
    audio_system.Initialize(AudioConfig{});
    
    // Measure audio response time
    auto start = std::chrono::high_resolution_clock::now();
    auto source_id = audio_system.PlaySound(test_sound_id, Vec3{0, 0, 0});
    // Wait for audio callback to confirm playback started
    WaitForAudioPlayback(source_id);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(latency, 10); // Under 10ms
    EXPECT_TRUE(source_id.has_value());
}

TEST(AudioTest, ConcurrentSources) {
    auto audio_system = engine::audio::AudioSystem{};
    audio_system.Initialize(AudioConfig{});
    
    // Play many simultaneous sounds
    std::vector<AudioSourceId> sources;
    for (int i = 0; i < 64; ++i) {
        auto source_id = audio_system.PlaySound(test_sound_id, 
            Vec3{static_cast<float>(i), 0, 0});
        ASSERT_TRUE(source_id.has_value());
        sources.push_back(*source_id);
    }
    
    // Verify all sources are playing
    auto stats = audio_system.GetAudioStats();
    EXPECT_EQ(stats.active_sources, 64);
    EXPECT_LT(stats.cpu_usage_percent, 5.0f); // Under 5% CPU
}

TEST(AudioTest, SpatialAccuracy) {
    auto audio_system = engine::audio::AudioSystem{};
    audio_system.Initialize(AudioConfig{});
    
    auto source_id = audio_system.PlaySound(test_sound_id, Vec3{10, 0, 0});
    ASSERT_TRUE(source_id.has_value());
    
    // Test distance attenuation
    audio_system.SetListenerTransform(Vec3{0, 0, 0}, Vec3{0, 0, -1}, Vec3{0, 1, 0});
    float volume_close = GetSourceVolume(*source_id);
    
    audio_system.SetListenerTransform(Vec3{-100, 0, 0}, Vec3{0, 0, -1}, Vec3{0, 1, 0});
    float volume_far = GetSourceVolume(*source_id);
    
    EXPECT_GT(volume_close, volume_far); // Distance attenuation working
    EXPECT_GT(volume_close, 0.1f); // Audible at close range
    EXPECT_LT(volume_far, 0.1f); // Quiet at far range
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Audio System (Estimated: 5 days)

- [ ] **Task 1.1**: Implement OpenAL/Web Audio backend abstraction
- [ ] **Task 1.2**: Add basic audio source management and playback
- [ ] **Task 1.3**: Implement audio asset loading and format support
- [ ] **Deliverable**: Basic audio playback working on all platforms

#### Phase 2: 3D Spatial Audio (Estimated: 4 days)

- [ ] **Task 2.1**: Implement 3D positioning and distance attenuation
- [ ] **Task 2.2**: Add doppler effects and velocity-based changes
- [ ] **Task 2.3**: Integrate with ECS Transform components
- [ ] **Deliverable**: Full 3D spatial audio with accurate positioning

#### Phase 3: Environmental Effects (Estimated: 3 days)

- [ ] **Task 3.1**: Implement reverb zones and environmental audio
- [ ] **Task 3.2**: Add audio occlusion and obstruction simulation
- [ ] **Task 3.3**: Create preset system for common environments
- [ ] **Deliverable**: Rich environmental audio effects

#### Phase 4: Performance Optimization (Estimated: 2 days)

- [ ] **Task 4.1**: Optimize audio thread and reduce latency
- [ ] **Task 4.2**: Implement audio source pooling and management
- [ ] **Task 4.3**: Add performance monitoring and profiling
- [ ] **Deliverable**: Production-ready audio performance

### File Structure

```
src/engine/audio/
├── audio.cppm                  // Primary module interface
├── audio_system.cpp           // Core audio management
├── audio_device.cpp           // Platform audio device abstraction
├── audio_source_pool.cpp      // Source management and pooling
├── spatial_audio.cpp          // 3D positioning and effects
├── audio_streaming.cpp        // Large file streaming support
├── environmental_audio.cpp    // Reverb and environmental effects
├── backends/
│   ├── openal_backend.cpp     // OpenAL implementation
│   └── webaudio_backend.cpp   // Web Audio implementation
└── tests/
    ├── audio_system_tests.cpp
    ├── spatial_audio_tests.cpp
    └── audio_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::audio`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with platform-specific backend implementations
- **Build Integration**: Links with OpenAL on desktop, Web Audio APIs on WebAssembly

### Testing Strategy

- **Unit Tests**: Mock audio devices for deterministic testing
- **Integration Tests**: Real audio hardware testing with automated verification
- **Performance Tests**: Latency measurement and concurrent source stress testing
- **Platform Tests**: Cross-platform audio format and backend testing

## Risk Assessment

### Technical Risks

| Risk                              | Probability | Impact | Mitigation                                            |
|-----------------------------------|-------------|--------|-------------------------------------------------------|
| **Audio Driver Issues**           | Medium      | High   | Fallback audio backends, driver compatibility testing |
| **WebAssembly Audio Limitations** | High        | Medium | Feature detection, graceful degradation for Web Audio |
| **Real-time Threading**           | Medium      | High   | Audio thread priority management, buffer size tuning  |
| **Memory Fragmentation**          | Low         | Medium | Audio buffer pooling, pre-allocated audio memory      |

### Integration Risks

- **ECS Performance**: Risk that audio component updates impact frame rate
    - *Mitigation*: Lock-free command queues, audio thread isolation
- **Asset Loading**: Risk of audio loading blocking main thread
    - *Mitigation*: Streaming audio assets, asynchronous loading

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (timing, threading primitives)
    - engine.ecs (component access, entity queries)
    - engine.assets (audio file loading and management)

- **Optional Systems**:
    - engine.profiling (audio performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 atomic operations, threading
- **Audio APIs**: OpenAL for desktop, Web Audio API for WebAssembly

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.assets
- **vcpkg Packages**: openal-soft (for desktop builds)
- **Platform-Specific**: OpenAL libraries, Web Audio for WASM builds
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.assets

### Asset Pipeline Dependencies

- **Audio Formats**: OGG Vorbis (primary), WAV (uncompressed), MP3 (compatibility)
- **Configuration Files**: Audio settings in JSON format following existing patterns
- **Resource Loading**: Both streaming (large files) and preloaded (small effects) support
