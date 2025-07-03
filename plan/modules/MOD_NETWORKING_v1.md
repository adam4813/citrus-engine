# MOD_NETWORKING_v1

> **Cross-Platform Network Infrastructure - Engine Module**

## Executive Summary

The `engine.networking` module provides a comprehensive cross-platform networking system supporting reliable and
unreliable communication, client-server architecture, peer-to-peer networking, and WebAssembly compatibility with
automatic serialization, network prediction, lag compensation, and secure communication. This module enables multiplayer
functionality for the Colony Game Engine including real-time synchronization, distributed simulation, and collaborative
gameplay across desktop and WebAssembly platforms with efficient bandwidth usage and robust connection management.

## Scope and Objectives

### In Scope

- [ ] Cross-platform networking (TCP/UDP sockets, WebSockets, WebRTC)
- [ ] Client-server and peer-to-peer networking architectures
- [ ] Automatic message serialization and deserialization
- [ ] Network prediction and lag compensation
- [ ] Connection management with reconnection and heartbeats
- [ ] Bandwidth optimization and message compression

### Out of Scope

- [ ] Advanced anti-cheat and server validation systems
- [ ] Voice chat and real-time audio streaming
- [ ] Complex network topology discovery and routing
- [ ] Enterprise-level load balancing and scaling

### Primary Objectives

1. **Performance**: Support 100+ concurrent connections with <100ms latency
2. **Reliability**: Maintain stable connections with automatic reconnection
3. **Compatibility**: Seamless networking across desktop and WebAssembly platforms

### Secondary Objectives

- Bandwidth usage under 10KB/s per player for typical gameplay
- Connection establishment under 500ms on stable networks
- Message delivery guarantee of 99.9% for critical game state

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages networked entity replication and ownership
- **Entity Queries**: Queries entities with NetworkEntity, NetworkTransform, or NetworkState components
- **Component Dependencies**: Requires NetworkEntity for replication; optional components for specific data sync

#### Component Design

```cpp
// Networking-related components for ECS integration
struct NetworkEntity {
    std::uint32_t network_id{0};
    ConnectionId owner_connection{0};
    NetworkAuthority authority{NetworkAuthority::Server};
    bool is_replicated{true};
    float replication_frequency{20.0f}; // Hz
    std::uint32_t last_update_frame{0};
};

struct NetworkTransform {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float rotation{0.0f};
    float angular_velocity{0.0f};
    std::uint32_t last_sync_frame{0};
    bool needs_interpolation{true};
};

struct NetworkState {
    std::unordered_map<std::string, NetworkVariable> variables;
    std::bitset<64> dirty_flags; // Track which variables changed
    std::uint32_t state_version{0};
    bool is_authoritative{false};
};

struct NetworkConnection {
    ConnectionId connection_id{0};
    NetworkAddress remote_address;
    ConnectionState state{ConnectionState::Disconnected};
    float ping{0.0f};
    std::uint32_t bytes_sent{0};
    std::uint32_t bytes_received{0};
    std::chrono::steady_clock::time_point last_heartbeat;
};

// Component traits for networking
template<>
struct ComponentTraits<NetworkEntity> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};

template<>
struct ComponentTraits<NetworkTransform> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 5000;
};
```

#### System Integration

- **System Dependencies**: Runs after game logic updates, coordinates with physics and rendering
- **Component Access Patterns**: Read-write access to network components; replicates data to remote clients
- **Inter-System Communication**: Provides network events for gameplay and state synchronization

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Network batch - executes on dedicated network thread for low latency
- **Thread Safety**: Network I/O isolated to network thread; message queues are thread-safe
- **Data Dependencies**: Reads game state; writes to network buffers and remote connections

#### Parallel Execution Strategy

```cpp
// Multi-threaded networking with dedicated I/O thread
class NetworkSystem : public ISystem {
public:
    // Main thread: Collect network data for replication
    void PrepareNetworkUpdates(const ComponentManager& components,
                              std::span<EntityId> entities,
                              const ThreadContext& context) override;
    
    // Network thread: Process I/O and message handling
    void ProcessNetworkIO(const ThreadContext& context);
    
    // Main thread: Apply received network updates to ECS
    void ApplyNetworkUpdates(const ComponentManager& components,
                            std::span<EntityId> entities,
                            const ThreadContext& context);

private:
    struct NetworkMessage {
        MessageType type;
        ConnectionId connection_id;
        std::vector<std::uint8_t> payload;
        std::chrono::steady_clock::time_point timestamp;
        MessagePriority priority;
    };
    
    // Lock-free queues for thread communication
    moodycamel::ConcurrentQueue<NetworkMessage> outbound_queue_;
    moodycamel::ConcurrentQueue<NetworkMessage> inbound_queue_;
    std::atomic<bool> network_thread_running_{false};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Network messages stored contiguously for efficient serialization
- **Memory Ordering**: Message queues use acquire-release semantics for thread safety
- **Lock-Free Sections**: Message serialization and connection management are lock-free

### Public APIs

#### Primary Interface: `NetworkManagerInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <functional>

namespace engine::networking {

template<typename T>
concept NetworkSerializable = requires(T t, NetworkSerializer& serializer) {
    { t.Serialize(serializer) } -> std::convertible_to<bool>;
    { t.Deserialize(serializer) } -> std::convertible_to<bool>;
};

class NetworkManagerInterface {
public:
    [[nodiscard]] auto Initialize(const NetworkConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Server management
    [[nodiscard]] auto StartServer(std::uint16_t port, std::uint32_t max_clients = 100) -> bool;
    void StopServer() noexcept;
    [[nodiscard]] auto IsServerRunning() const noexcept -> bool;
    
    // Client management
    [[nodiscard]] auto ConnectToServer(const NetworkAddress& server_address) -> std::optional<ConnectionId>;
    void DisconnectFromServer() noexcept;
    [[nodiscard]] auto IsConnectedToServer() const noexcept -> bool;
    
    // Message handling
    template<NetworkSerializable T>
    void SendMessage(ConnectionId connection_id, const T& message, MessageReliability reliability = MessageReliability::Reliable);
    
    template<NetworkSerializable T>
    void BroadcastMessage(const T& message, MessageReliability reliability = MessageReliability::Reliable);
    
    template<NetworkSerializable T>
    [[nodiscard]] auto RegisterMessageHandler(std::function<void(ConnectionId, const T&)> handler) -> CallbackId;
    
    void UnregisterMessageHandler(CallbackId id) noexcept;
    
    // Connection management
    void DisconnectClient(ConnectionId connection_id) noexcept;
    [[nodiscard]] auto GetConnectedClients() const -> std::vector<ConnectionId>;
    [[nodiscard]] auto GetConnectionInfo(ConnectionId connection_id) const -> std::optional<ConnectionInfo>;
    
    // Network statistics
    [[nodiscard]] auto GetNetworkStats() const -> NetworkStatistics;
    void SetBandwidthLimit(std::uint32_t bytes_per_second) noexcept;
    
    // P2P networking
    [[nodiscard]] auto CreateP2PSession() -> std::optional<P2PSessionId>;
    void JoinP2PSession(P2PSessionId session_id, const NetworkAddress& host_address);
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<NetworkManagerImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::networking
```

#### Scripting Interface Requirements

```cpp
// Networking scripting interface for game-specific network logic
class NetworkingScriptInterface {
public:
    // Type-safe network communication from scripts
    void SendMessageToServer(std::string_view message_type, const std::string& data);
    void SendMessageToClient(std::uint32_t connection_id, std::string_view message_type, const std::string& data);
    void BroadcastMessage(std::string_view message_type, const std::string& data);
    
    // Connection management
    [[nodiscard]] auto IsConnectedToServer() const -> bool;
    [[nodiscard]] auto IsServerRunning() const -> bool;
    [[nodiscard]] auto GetConnectedClientCount() const -> std::uint32_t;
    
    // Network state queries
    [[nodiscard]] auto GetPing() const -> float;
    [[nodiscard]] auto GetBytesPerSecond() const -> std::uint32_t;
    
    // Event registration
    [[nodiscard]] auto OnClientConnected(std::string_view callback_function) -> bool;
    [[nodiscard]] auto OnClientDisconnected(std::string_view callback_function) -> bool;
    [[nodiscard]] auto OnMessageReceived(std::string_view message_type, std::string_view callback_function) -> bool;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<NetworkManagerInterface> network_manager_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Cross-Platform Communication**: Seamless networking between desktop and WebAssembly clients
- [ ] **Reliable Message Delivery**: Guarantee delivery of critical game state messages
- [ ] **Connection Management**: Automatic reconnection and heartbeat monitoring

### Performance Requirements

- [ ] **Concurrent Connections**: Support 100+ simultaneous connections on server
- [ ] **Latency**: Maintain <100ms round-trip time for local network connections
- [ ] **Bandwidth**: Keep bandwidth usage under 10KB/s per player for typical gameplay
- [ ] **Throughput**: Process 1,000+ messages per second without performance degradation

### Quality Requirements

- [ ] **Reliability**: 99.9% message delivery rate for reliable messages
- [ ] **Maintainability**: All networking code covered by automated tests
- [ ] **Testability**: Mock network interfaces for deterministic testing
- [ ] **Documentation**: Complete networking guide with protocol documentation

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(NetworkingTest, ConcurrentConnections) {
    auto network_manager = engine::networking::NetworkManager{};
    network_manager.Initialize(NetworkConfig{});
    
    // Start server
    ASSERT_TRUE(network_manager.StartServer(8080, 100));
    
    // Connect multiple clients
    std::vector<std::unique_ptr<NetworkClient>> clients;
    for (int i = 0; i < 100; ++i) {
        auto client = std::make_unique<NetworkClient>();
        auto connection_id = client->ConnectToServer(NetworkAddress{"localhost", 8080});
        ASSERT_TRUE(connection_id.has_value());
        clients.push_back(std::move(client));
    }
    
    // Verify all connections
    auto connected_clients = network_manager.GetConnectedClients();
    EXPECT_EQ(connected_clients.size(), 100);
    
    // Test message broadcasting
    TestMessage message{"Hello World"};
    auto start = std::chrono::high_resolution_clock::now();
    network_manager.BroadcastMessage(message);
    
    // Wait for all clients to receive message
    for (auto& client : clients) {
        WaitForMessage(*client, "Hello World");
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto broadcast_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(broadcast_time, 1000); // Under 1 second for 100 clients
}

TEST(NetworkingTest, MessageReliability) {
    auto server = engine::networking::NetworkManager{};
    auto client = engine::networking::NetworkClient{};
    
    server.Initialize(NetworkConfig{});
    client.Initialize(NetworkConfig{});
    
    ASSERT_TRUE(server.StartServer(8081));
    auto connection_id = client.ConnectToServer(NetworkAddress{"localhost", 8081});
    ASSERT_TRUE(connection_id.has_value());
    
    // Send many reliable messages
    std::atomic<int> messages_received{0};
    client.RegisterMessageHandler<TestMessage>([&](ConnectionId, const TestMessage& msg) {
        messages_received++;
    });
    
    const int message_count = 1000;
    for (int i = 0; i < message_count; ++i) {
        TestMessage message{std::to_string(i)};
        server.SendMessage(*connection_id, message, MessageReliability::Reliable);
    }
    
    // Wait for all messages
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (messages_received.load() < message_count && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_EQ(messages_received.load(), message_count); // 100% delivery rate
}

TEST(NetworkingTest, LatencyMeasurement) {
    auto server = engine::networking::NetworkManager{};
    auto client = engine::networking::NetworkClient{};
    
    server.Initialize(NetworkConfig{});
    client.Initialize(NetworkConfig{});
    
    ASSERT_TRUE(server.StartServer(8082));
    auto connection_id = client.ConnectToServer(NetworkAddress{"localhost", 8082});
    ASSERT_TRUE(connection_id.has_value());
    
    // Measure round-trip time
    std::vector<float> ping_times;
    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        PingMessage ping_msg{i};
        client.SendMessage(ping_msg);
        
        // Wait for pong response
        WaitForPongMessage(client, i);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto rtt = std::chrono::duration<float, std::milli>(end - start).count();
        ping_times.push_back(rtt);
    }
    
    auto avg_ping = std::accumulate(ping_times.begin(), ping_times.end(), 0.0f) / ping_times.size();
    EXPECT_LT(avg_ping, 100.0f); // Under 100ms average
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Networking Foundation (Estimated: 8 days)

- [ ] **Task 1.1**: Implement basic socket abstraction and cross-platform networking
- [ ] **Task 1.2**: Add message serialization and deserialization framework
- [ ] **Task 1.3**: Create connection management and heartbeat system
- [ ] **Deliverable**: Basic client-server communication working

#### Phase 2: Reliable Messaging (Estimated: 6 days)

- [ ] **Task 2.1**: Implement reliable message delivery with acknowledgments
- [ ] **Task 2.2**: Add message ordering and duplicate detection
- [ ] **Task 2.3**: Create bandwidth management and flow control
- [ ] **Deliverable**: Robust message delivery system

#### Phase 3: ECS Integration (Estimated: 5 days)

- [ ] **Task 3.1**: Create network entity replication system
- [ ] **Task 3.2**: Implement network transform synchronization
- [ ] **Task 3.3**: Add network state management and prediction
- [ ] **Deliverable**: Full ECS networking integration

#### Phase 4: Advanced Features (Estimated: 4 days)

- [ ] **Task 4.1**: Add WebAssembly networking support (WebSockets/WebRTC)
- [ ] **Task 4.2**: Implement P2P networking and discovery
- [ ] **Task 4.3**: Create network debugging and monitoring tools
- [ ] **Deliverable**: Production-ready networking with P2P support

### File Structure

```
src/engine/networking/
├── networking.cppm             // Primary module interface
├── network_manager.cpp         // Core network management
├── connection.cpp              // Connection handling and lifecycle
├── message_serialization.cpp   // Message encoding/decoding
├── reliable_delivery.cpp       // Reliable message delivery
├── network_prediction.cpp      // Client-side prediction
├── bandwidth_manager.cpp       // Traffic shaping and optimization
├── protocols/
│   ├── tcp_protocol.cpp       // TCP socket implementation
│   ├── udp_protocol.cpp       // UDP socket implementation
│   ├── websocket_protocol.cpp // WebSocket implementation
│   └── webrtc_protocol.cpp    // WebRTC implementation
├── p2p/
│   ├── p2p_discovery.cpp      // Peer discovery
│   └── p2p_session.cpp        // P2P session management
└── tests/
    ├── network_manager_tests.cpp
    ├── reliability_tests.cpp
    └── networking_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::networking`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with protocol-specific implementations
- **Build Integration**: Links with platform-specific networking libraries

### Testing Strategy

- **Unit Tests**: Mock sockets and connections for deterministic networking tests
- **Integration Tests**: Real network testing with multiple processes
- **Performance Tests**: Load testing with many concurrent connections
- **Platform Tests**: Cross-platform networking compatibility validation

## Risk Assessment

### Technical Risks

| Risk                        | Probability | Impact | Mitigation                                   |
|-----------------------------|-------------|--------|----------------------------------------------|
| **WebAssembly Limitations** | High        | Medium | WebSocket fallback, feature detection        |
| **Firewall/NAT Issues**     | Medium      | High   | STUN/TURN servers, UPnP support              |
| **Message Loss**            | Medium      | High   | Reliable delivery protocols, acknowledgments |
| **Performance Scaling**     | Medium      | Medium | Connection pooling, bandwidth management     |

### Integration Risks

- **ECS Performance**: Risk that network replication impacts frame rate
    - *Mitigation*: Efficient serialization, delta compression, update frequency limits
- **Platform Differences**: Risk of networking behavior differences across platforms
    - *Mitigation*: Consistent protocol implementation, extensive cross-platform testing

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (socket abstraction, threading)
    - engine.ecs (entity replication, component synchronization)
    - engine.threading (network I/O thread, message queues)

- **Optional Systems**:
    - engine.profiling (network performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 networking (when available), sockets, threading
- **Platform APIs**: BSD sockets, WinSock, WebSocket APIs, WebRTC

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.threading
- **vcpkg Packages**: WebRTC libraries for P2P support
- **Platform-Specific**: Socket libraries, WebAssembly networking APIs
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.threading

### Asset Pipeline Dependencies

- **Network Configuration**: JSON-based networking configuration files
- **Protocol Definitions**: Message format definitions and serialization schemas
- **Resource Loading**: Network configuration loaded through engine.assets
