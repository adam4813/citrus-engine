# SYS_NETWORKING_v1

> **System-Level Design Document for Cross-Platform Network Infrastructure**

## Executive Summary

This document defines a comprehensive networking system for the modern C++20 game engine, designed to provide flexible
network communication infrastructure that supports multiple networking models (client-server, peer-to-peer, hybrid)
without favoring any particular approach. The system emphasizes cross-platform compatibility, headless server operation,
and seamless ECS integration while maintaining high performance for real-time multiplayer scenarios. The design
abstracts platform-specific networking APIs and provides a unified interface that works identically across Windows,
Linux, and WebAssembly platforms.

## Scope and Objectives

### In Scope

- [ ] Cross-platform socket abstraction for TCP and UDP communication
- [ ] Multiple networking model support (client-server, peer-to-peer, hybrid architectures)
- [ ] Headless server operation with dedicated server executable support
- [ ] ECS integration with networked components and automatic state synchronization
- [ ] Message serialization and deserialization with automatic type handling
- [ ] Network message routing and reliable delivery mechanisms
- [ ] WebAssembly networking support using WebRTC and WebSocket protocols
- [ ] Network security features including encryption and authentication
- [ ] Bandwidth optimization through delta compression and batching
- [ ] Scripting interface for network event handling and custom protocols

### Out of Scope

- [ ] High-level game-specific networking protocols (lobbies, matchmaking, leaderboards)
- [ ] Voice chat or streaming media transmission capabilities
- [ ] Advanced anti-cheat or server-side validation systems
- [ ] Network-aware asset distribution or content delivery networks
- [ ] Real-time streaming or live broadcasting functionality
- [ ] Platform-specific networking optimizations (console networking APIs)

### Primary Objectives

1. **Flexibility**: Support multiple networking models without architectural bias toward any approach
2. **Cross-Platform**: Identical networking API and behavior across Windows, Linux, and WebAssembly
3. **Performance**: Handle 100+ networked entities with 20Hz update rates at sub-50ms latency
4. **ECS Integration**: Seamless network replication of ECS components with minimal overhead
5. **Headless Operation**: Full server functionality without graphics, audio, or input dependencies

### Secondary Objectives

- Comprehensive debugging and monitoring tools for network performance analysis
- Hot-reload support for network configuration and protocol definitions
- Future extensibility for advanced networking features and optimization techniques
- Memory-efficient operation suitable for WebAssembly heap constraints
- Integration with existing scripting system for custom network logic

## Architecture/Design

### High-Level Overview

```
Network System Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                Network Application Layer                        │
├─────────────────────────────────────────────────────────────────┤
│  Game Protocol │ ECS Sync     │ Custom      │ Script         │
│  Handlers      │ System       │ Messages    │ Integration    │
│                │              │             │                │
│  High-Level    │ Component    │ Application │ Lua/Python/    │
│  Message       │ Replication  │ Specific    │ AngelScript    │
│  Processing    │ & Authority  │ Protocols   │ Network APIs   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│               Network Transport Layer                           │
├─────────────────────────────────────────────────────────────────┤
│  Message       │ Reliable     │ Connection  │ Bandwidth      │
│  Routing       │ Delivery     │ Management  │ Management     │
│                │              │             │                │
│  Client-Server │ TCP/UDP      │ Connection  │ Delta          │
│  P2P Support   │ Abstraction  │ Lifecycle   │ Compression    │
│  Hybrid Models │ Ordering     │ Heartbeat   │ Batching       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│             Platform Network Abstraction                       │
├─────────────────────────────────────────────────────────────────┤
│  Native        │ WebAssembly  │ Serialization │ Security       │
│  Sockets       │ Support      │ System        │ Layer          │
│                │              │               │                │
│  Windows       │ WebRTC       │ Binary        │ Encryption     │
│  Winsock       │ WebSocket    │ JSON          │ Authentication │
│  Linux/POSIX   │ Fallbacks    │ Compression   │ Message        │
│  BSD Sockets   │              │               │ Validation     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                Foundation Integration                           │
├─────────────────────────────────────────────────────────────────┤
│  Platform      │ Threading    │ ECS Core     │ Scripting      │
│  Abstraction   │ System       │              │ Interface      │
│                │              │              │                │
│  Socket APIs   │ Network I/O  │ Component    │ Network Event  │
│  Timing        │ Thread Pool  │ Queries      │ Handlers       │
│  Memory Mgmt   │ Job System   │ Systems      │ Custom Logic   │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Network Transport Layer

- **Purpose**: Platform-agnostic network communication with support for multiple transport protocols
- **Responsibilities**: Socket management, connection handling, message delivery, protocol abstraction
- **Key Classes/Interfaces**: `NetworkTransport`, `NetworkConnection`, `NetworkMessage`, `NetworkProtocol`
- **Data Flow**: Application messages → Transport encoding → Platform sockets → Network transmission

#### Component 2: ECS Network Integration

- **Purpose**: Automatic synchronization of ECS components across network connections with authority management
- **Responsibilities**: Component replication, authority resolution, network entity management, state synchronization
- **Key Classes/Interfaces**: `NetworkSystem`, `NetworkComponent`, `NetworkAuthority`, `ComponentReplicator`
- **Data Flow**: Component changes → Authority validation → Network serialization → Remote component updates

#### Component 3: Flexible Network Architecture Support

- **Purpose**: Support for multiple networking models without architectural bias toward any specific approach
- **Responsibilities**: Client-server coordination, peer-to-peer networking, hybrid model support, topology management
- **Key Classes/Interfaces**: `NetworkTopology`, `ServerManager`, `ClientManager`, `PeerManager`
- **Data Flow**: Network role determination → Topology configuration → Connection establishment → Message routing

#### Component 4: Cross-Platform Network Implementation

- **Purpose**: Unified networking API abstracting platform-specific socket implementations
- **Responsibilities**: Socket abstraction, WebAssembly networking, platform capability detection, fallback handling
- **Key Classes/Interfaces**: `PlatformNetworking`, `SocketManager`, `WebRTCProvider`, `WebSocketProvider`
- **Data Flow**: Network requests → Platform detection → Appropriate implementation → Unified response

### Network Transport Architecture

#### Core Networking Interface

```cpp
namespace engine::networking {

// Network message types for type-safe communication
enum class NetworkMessageType : uint16_t {
    // System messages
    Handshake = 0x0001,
    Disconnect = 0x0002,
    Heartbeat = 0x0003,
    
    // ECS synchronization
    ComponentUpdate = 0x0100,
    EntityCreate = 0x0101,
    EntityDestroy = 0x0102,
    
    // Custom application messages
    GameEvent = 0x0200,
    UserDefined = 0x1000  // User messages start from this value
};

// Network message container with automatic serialization
class NetworkMessage {
public:
    explicit NetworkMessage(NetworkMessageType type) : type_(type) {}
    
    // Template-based data serialization
    template<typename T>
    void WriteData(const T& data) {
        static_assert(std::is_trivially_copyable_v<T> || has_serialize_method_v<T>, 
                     "Type must be trivially copyable or have serialize() method");
        SerializeData(data);
    }
    
    template<typename T>
    [[nodiscard]] auto ReadData() -> std::optional<T> {
        return DeserializeData<T>();
    }
    
    // Message properties
    [[nodiscard]] auto GetType() const -> NetworkMessageType { return type_; }
    [[nodiscard]] auto GetSize() const -> size_t { return data_.size(); }
    [[nodiscard]] auto GetTimestamp() const -> std::chrono::steady_clock::time_point { return timestamp_; }
    [[nodiscard]] auto GetData() const -> std::span<const uint8_t> { return data_; }
    
    // Reliability and ordering
    void SetReliable(bool reliable) { is_reliable_ = reliable; }
    void SetOrdered(bool ordered) { is_ordered_ = ordered; }
    [[nodiscard]] auto IsReliable() const -> bool { return is_reliable_; }
    [[nodiscard]] auto IsOrdered() const -> bool { return is_ordered_; }

private:
    NetworkMessageType type_;
    std::vector<uint8_t> data_;
    std::chrono::steady_clock::time_point timestamp_ = std::chrono::steady_clock::now();
    bool is_reliable_ = true;
    bool is_ordered_ = true;
    
    template<typename T>
    void SerializeData(const T& data);
    
    template<typename T>
    [[nodiscard]] auto DeserializeData() -> std::optional<T>;
};

// Network connection abstraction
class NetworkConnection {
public:
    enum class State { Disconnected, Connecting, Connected, Disconnecting };
    enum class Type { TCP, UDP, WebRTC, WebSocket };
    
    explicit NetworkConnection(Type type) : type_(type) {}
    virtual ~NetworkConnection() = default;
    
    // Connection management
    [[nodiscard]] virtual auto Connect(const NetworkAddress& address) -> bool = 0;
    virtual void Disconnect() = 0;
    [[nodiscard]] virtual auto GetState() const -> State = 0;
    
    // Message transmission
    [[nodiscard]] virtual auto SendMessage(const NetworkMessage& message) -> bool = 0;
    [[nodiscard]] virtual auto ReceiveMessages() -> std::vector<NetworkMessage> = 0;
    
    // Connection properties
    [[nodiscard]] auto GetType() const -> Type { return type_; }
    [[nodiscard]] auto GetRemoteAddress() const -> const NetworkAddress& { return remote_address_; }
    [[nodiscard]] auto GetConnectionId() const -> ConnectionId { return connection_id_; }
    
    // Statistics and monitoring
    [[nodiscard]] virtual auto GetStatistics() const -> NetworkStatistics = 0;
    [[nodiscard]] virtual auto GetLatency() const -> std::chrono::milliseconds = 0;

protected:
    Type type_;
    NetworkAddress remote_address_;
    ConnectionId connection_id_;
    State state_ = State::Disconnected;
};

// Network address abstraction for cross-platform addressing
struct NetworkAddress {
    std::string hostname;
    uint16_t port;
    bool is_ipv6 = false;
    
    explicit NetworkAddress(std::string host, uint16_t port_num, bool ipv6 = false)
        : hostname(std::move(host)), port(port_num), is_ipv6(ipv6) {}
    
    [[nodiscard]] auto ToString() const -> std::string {
        return is_ipv6 ? std::format("[{}]:{}", hostname, port) 
                       : std::format("{}:{}", hostname, port);
    }
    
    [[nodiscard]] auto operator==(const NetworkAddress& other) const -> bool {
        return hostname == other.hostname && port == other.port && is_ipv6 == other.is_ipv6;
    }
};

// Network statistics for monitoring and debugging
struct NetworkStatistics {
    uint64_t bytes_sent = 0;
    uint64_t bytes_received = 0;
    uint64_t messages_sent = 0;
    uint64_t messages_received = 0;
    uint64_t messages_lost = 0;
    std::chrono::milliseconds average_latency{0};
    std::chrono::steady_clock::time_point connection_start_time;
    
    [[nodiscard]] auto GetUptime() const -> std::chrono::milliseconds {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - connection_start_time);
    }
    
    [[nodiscard]] auto GetPacketLossRate() const -> float {
        if (messages_sent == 0) return 0.0f;
        return static_cast<float>(messages_lost) / static_cast<float>(messages_sent);
    }
};

} // namespace engine::networking
```

#### Flexible Network Topology Support

```cpp
namespace engine::networking {

// Network topology manager supporting multiple networking models
class NetworkTopology {
public:
    enum class TopologyType { ClientServer, PeerToPeer, Hybrid };
    
    explicit NetworkTopology(TopologyType type) : topology_type_(type) {}
    virtual ~NetworkTopology() = default;
    
    // Topology management
    [[nodiscard]] virtual auto Initialize() -> bool = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual auto GetTopologyType() const -> TopologyType { return topology_type_; }
    
    // Connection management
    [[nodiscard]] virtual auto CreateConnection(const NetworkAddress& address) -> std::shared_ptr<NetworkConnection> = 0;
    virtual void CloseConnection(ConnectionId connection_id) = 0;
    [[nodiscard]] virtual auto GetConnections() const -> std::vector<std::shared_ptr<NetworkConnection>> = 0;
    
    // Message routing
    virtual void BroadcastMessage(const NetworkMessage& message) = 0;
    virtual void SendToConnection(ConnectionId connection_id, const NetworkMessage& message) = 0;
    [[nodiscard]] virtual auto ReceiveMessages() -> std::vector<std::pair<ConnectionId, NetworkMessage>> = 0;
    
    // Topology-specific operations
    [[nodiscard]] virtual auto SupportsHeadlessOperation() const -> bool = 0;
    [[nodiscard]] virtual auto GetMaxConnections() const -> size_t = 0;

protected:
    TopologyType topology_type_;
};

// Client-server topology implementation
class ClientServerTopology : public NetworkTopology {
public:
    explicit ClientServerTopology(bool is_server) 
        : NetworkTopology(TopologyType::ClientServer), is_server_(is_server) {}
    
    [[nodiscard]] auto Initialize() -> bool override {
        if (is_server_) {
            return InitializeServer();
        } else {
            return InitializeClient();
        }
    }
    
    void Shutdown() override {
        if (is_server_) {
            ShutdownServer();
        } else {
            ShutdownClient();
        }
    }
    
    // Server-specific functionality
    [[nodiscard]] auto StartServer(const NetworkAddress& bind_address) -> bool {
        if (!is_server_) return false;
        
        server_socket_ = CreateServerSocket(bind_address);
        if (!server_socket_) return false;
        
        is_listening_ = true;
        StartAcceptingConnections();
        return true;
    }
    
    // Client-specific functionality
    [[nodiscard]] auto ConnectToServer(const NetworkAddress& server_address) -> bool {
        if (is_server_) return false;
        
        auto connection = CreateConnection(server_address);
        if (!connection || !connection->Connect(server_address)) {
            return false;
        }
        
        server_connection_ = connection;
        return true;
    }
    
    // Authority management for server
    void SetAuthoritative(bool authoritative) { is_authoritative_ = authoritative; }
    [[nodiscard]] auto IsAuthoritative() const -> bool { return is_authoritative_; }
    
    [[nodiscard]] auto SupportsHeadlessOperation() const -> bool override { return is_server_; }
    [[nodiscard]] auto GetMaxConnections() const -> size_t override { return is_server_ ? 64 : 1; }

private:
    bool is_server_;
    bool is_listening_ = false;
    bool is_authoritative_ = false;
    std::unique_ptr<ServerSocket> server_socket_;
    std::shared_ptr<NetworkConnection> server_connection_; // For clients
    std::vector<std::shared_ptr<NetworkConnection>> client_connections_; // For server
    
    [[nodiscard]] auto InitializeServer() -> bool;
    [[nodiscard]] auto InitializeClient() -> bool;
    void ShutdownServer();
    void ShutdownClient();
    void StartAcceptingConnections();
    [[nodiscard]] auto CreateServerSocket(const NetworkAddress& bind_address) -> std::unique_ptr<ServerSocket>;
};

// Peer-to-peer topology implementation
class PeerToPeerTopology : public NetworkTopology {
public:
    PeerToPeerTopology() : NetworkTopology(TopologyType::PeerToPeer) {}
    
    [[nodiscard]] auto Initialize() -> bool override {
        // Initialize P2P networking stack
        return InitializePeerDiscovery();
    }
    
    // Peer discovery and connection
    [[nodiscard]] auto DiscoverPeers() -> std::vector<NetworkAddress> {
        // Implementation depends on discovery mechanism (LAN broadcast, signaling server, etc.)
        return discovered_peers_;
    }
    
    [[nodiscard]] auto ConnectToPeer(const NetworkAddress& peer_address) -> std::shared_ptr<NetworkConnection> {
        auto connection = CreateConnection(peer_address);
        if (connection && connection->Connect(peer_address)) {
            peer_connections_.push_back(connection);
            return connection;
        }
        return nullptr;
    }
    
    // Mesh networking support
    void EnableMeshNetworking(bool enable) { mesh_networking_enabled_ = enable; }
    [[nodiscard]] auto IsMeshNetworkingEnabled() const -> bool { return mesh_networking_enabled_; }
    
    [[nodiscard]] auto SupportsHeadlessOperation() const -> bool override { return true; }
    [[nodiscard]] auto GetMaxConnections() const -> size_t override { return 32; }

private:
    std::vector<NetworkAddress> discovered_peers_;
    std::vector<std::shared_ptr<NetworkConnection>> peer_connections_;
    bool mesh_networking_enabled_ = false;
    
    [[nodiscard]] auto InitializePeerDiscovery() -> bool;
};

// Hybrid topology supporting both client-server and peer-to-peer elements
class HybridTopology : public NetworkTopology {
public:
    HybridTopology() : NetworkTopology(TopologyType::Hybrid) {}
    
    // Combine server connections for centralized services with peer connections for gameplay
    void AddServerConnection(const NetworkAddress& server_address) {
        auto connection = CreateConnection(server_address);
        if (connection && connection->Connect(server_address)) {
            server_connections_.push_back(connection);
        }
    }
    
    void AddPeerConnection(const NetworkAddress& peer_address) {
        auto connection = CreateConnection(peer_address);
        if (connection && connection->Connect(peer_address)) {
            peer_connections_.push_back(connection);
        }
    }
    
    // Route messages based on message type and destination
    void RouteMessage(const NetworkMessage& message, MessageDestination destination) {
        switch (destination) {
            case MessageDestination::Server:
                SendToServers(message);
                break;
            case MessageDestination::Peers:
                SendToPeers(message);
                break;
            case MessageDestination::All:
                SendToServers(message);
                SendToPeers(message);
                break;
        }
    }
    
    [[nodiscard]] auto SupportsHeadlessOperation() const -> bool override { return true; }
    [[nodiscard]] auto GetMaxConnections() const -> size_t override { return 96; } // 32 servers + 64 peers

private:
    std::vector<std::shared_ptr<NetworkConnection>> server_connections_;
    std::vector<std::shared_ptr<NetworkConnection>> peer_connections_;
    
    void SendToServers(const NetworkMessage& message);
    void SendToPeers(const NetworkMessage& message);
};

} // namespace engine::networking
```

### ECS Network Integration

#### Networked Component System

```cpp
namespace engine::networking {

// Component for entities that should be synchronized over the network
struct NetworkComponent {
    ConnectionId authority_connection = ConnectionId::Invalid; // Who has authority over this entity
    uint32_t network_id = 0;                                  // Unique network identifier
    bool is_local_authority = false;                          // Whether this client has authority
    float sync_frequency = 20.0f;                             // Synchronization frequency in Hz
    float last_sync_time = 0.0f;                              // Time of last synchronization
    
    // Synchronization flags
    bool sync_transform = true;                               // Sync position, rotation, scale
    bool sync_on_change_only = false;                         // Only sync when values change
    bool always_replicate = false;                           // Replicate even if not in view
    
    // Network compression settings
    float position_threshold = 0.01f;                        // Minimum position change to sync
    float rotation_threshold = 0.1f;                         // Minimum rotation change to sync (degrees)
    float scale_threshold = 0.01f;                           // Minimum scale change to sync
};

template<>
struct ComponentTraits<NetworkComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(ConnectionId);
};

// Authority component for managing network ownership
struct NetworkAuthorityComponent {
    enum class AuthorityType { Server, Client, Shared };
    
    AuthorityType authority_type = AuthorityType::Server;
    ConnectionId owner_connection = ConnectionId::Invalid;
    bool can_transfer_authority = true;
    std::chrono::steady_clock::time_point authority_timestamp;
    
    // Authority validation
    [[nodiscard]] auto HasAuthority(ConnectionId connection_id) const -> bool {
        return owner_connection == connection_id;
    }
    
    void TransferAuthority(ConnectionId new_owner) {
        if (can_transfer_authority) {
            owner_connection = new_owner;
            authority_timestamp = std::chrono::steady_clock::now();
        }
    }
};

template<>
struct ComponentTraits<NetworkAuthorityComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;
    static constexpr size_t alignment = alignof(ConnectionId);
};

// Network system for ECS integration
class NetworkSystem : public ISystem {
public:
    explicit NetworkSystem(std::shared_ptr<NetworkTopology> topology) 
        : topology_(std::move(topology)) {}
    
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                ComponentTypeId::GetId<TransformComponent>(),
                ComponentTypeId::GetId<NetworkComponent>(),
                ComponentTypeId::GetId<NetworkAuthorityComponent>()
            },
            .write_components = {
                ComponentTypeId::GetId<TransformComponent>(),
                ComponentTypeId::GetId<NetworkComponent>()
            },
            .execution_phase = ExecutionPhase::PostUpdate, // After game logic
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Process incoming network messages
        ProcessIncomingMessages(world);
        
        // Send outgoing component updates
        ProcessOutgoingUpdates(world, delta_time);
        
        // Handle authority changes
        ProcessAuthorityChanges(world);
        
        // Clean up disconnected entities
        ProcessDisconnections(world);
    }

private:
    std::shared_ptr<NetworkTopology> topology_;
    std::unordered_map<uint32_t, EntityId> network_id_to_entity_;
    std::unordered_map<EntityId, uint32_t> entity_to_network_id_;
    uint32_t next_network_id_ = 1;
    
    void ProcessIncomingMessages(World& world) {
        auto messages = topology_->ReceiveMessages();
        
        for (const auto& [connection_id, message] : messages) {
            switch (message.GetType()) {
                case NetworkMessageType::ComponentUpdate:
                    HandleComponentUpdate(world, connection_id, message);
                    break;
                case NetworkMessageType::EntityCreate:
                    HandleEntityCreate(world, connection_id, message);
                    break;
                case NetworkMessageType::EntityDestroy:
                    HandleEntityDestroy(world, connection_id, message);
                    break;
                default:
                    // Forward to application-specific handlers
                    break;
            }
        }
    }
    
    void ProcessOutgoingUpdates(World& world, float delta_time) {
        world.ForEachComponent<NetworkComponent, TransformComponent>(
            [this, delta_time](EntityId entity, NetworkComponent& network, const TransformComponent& transform) {
                if (!network.is_local_authority) return;
                
                // Check if it's time to sync
                network.last_sync_time += delta_time;
                if (network.last_sync_time < (1.0f / network.sync_frequency)) {
                    return;
                }
                
                // Create component update message
                if (ShouldSyncTransform(network, transform)) {
                    auto message = CreateComponentUpdateMessage(entity, network, transform);
                    topology_->BroadcastMessage(message);
                    network.last_sync_time = 0.0f;
                }
            });
    }
    
    void ProcessAuthorityChanges(World& world) {
        // Handle authority transfer requests and validation
        world.ForEachComponent<NetworkAuthorityComponent>(
            [this](EntityId entity, NetworkAuthorityComponent& authority) {
                // Validate authority timestamps and handle conflicts
                ValidateAuthority(entity, authority);
            });
    }
    
    void ProcessDisconnections(World& world) {
        // Clean up entities owned by disconnected clients
        auto connections = topology_->GetConnections();
        std::unordered_set<ConnectionId> active_connections;
        
        for (const auto& connection : connections) {
            active_connections.insert(connection->GetConnectionId());
        }
        
        world.ForEachComponent<NetworkComponent>([this, &active_connections](EntityId entity, const NetworkComponent& network) {
            if (active_connections.find(network.authority_connection) == active_connections.end()) {
                // Authority connection is no longer active, handle cleanup
                HandleEntityDisconnection(entity, network);
            }
        });
    }
    
    void HandleComponentUpdate(World& world, ConnectionId connection_id, const NetworkMessage& message);
    void HandleEntityCreate(World& world, ConnectionId connection_id, const NetworkMessage& message);
    void HandleEntityDestroy(World& world, ConnectionId connection_id, const NetworkMessage& message);
    void HandleEntityDisconnection(EntityId entity, const NetworkComponent& network);
    
    [[nodiscard]] auto ShouldSyncTransform(const NetworkComponent& network, const TransformComponent& transform) const -> bool;
    [[nodiscard]] auto CreateComponentUpdateMessage(EntityId entity, const NetworkComponent& network, const TransformComponent& transform) const -> NetworkMessage;
    void ValidateAuthority(EntityId entity, NetworkAuthorityComponent& authority);
};

} // namespace engine::networking
```

### Cross-Platform Implementation

#### Platform-Specific Networking

```cpp
namespace engine::networking {

// Abstract platform networking interface
class PlatformNetworking {
public:
    virtual ~PlatformNetworking() = default;
    
    // Socket management
    [[nodiscard]] virtual auto CreateTCPSocket() -> std::unique_ptr<NetworkConnection> = 0;
    [[nodiscard]] virtual auto CreateUDPSocket() -> std::unique_ptr<NetworkConnection> = 0;
    [[nodiscard]] virtual auto CreateServerSocket(const NetworkAddress& bind_address) -> std::unique_ptr<ServerSocket> = 0;
    
    // Platform capabilities
    [[nodiscard]] virtual auto SupportsTCP() const -> bool = 0;
    [[nodiscard]] virtual auto SupportsUDP() const -> bool = 0;
    [[nodiscard]] virtual auto SupportsIPv6() const -> bool = 0;
    [[nodiscard]] virtual auto SupportsWebRTC() const -> bool = 0;
    
    // Address resolution
    [[nodiscard]] virtual auto ResolveAddress(const std::string& hostname) -> std::optional<NetworkAddress> = 0;
    [[nodiscard]] virtual auto GetLocalAddresses() -> std::vector<NetworkAddress> = 0;
};

// Native platform implementation (Windows/Linux)
class NativePlatformNetworking : public PlatformNetworking {
public:
    NativePlatformNetworking() {
        #ifdef _WIN32
            InitializeWinsock();
        #endif
    }
    
    ~NativePlatformNetworking() override {
        #ifdef _WIN32
            CleanupWinsock();
        #endif
    }
    
    [[nodiscard]] auto CreateTCPSocket() -> std::unique_ptr<NetworkConnection> override {
        return std::make_unique<NativeTCPConnection>();
    }
    
    [[nodiscard]] auto CreateUDPSocket() -> std::unique_ptr<NetworkConnection> override {
        return std::make_unique<NativeUDPConnection>();
    }
    
    [[nodiscard]] auto CreateServerSocket(const NetworkAddress& bind_address) -> std::unique_ptr<ServerSocket> override {
        return std::make_unique<NativeServerSocket>(bind_address);
    }
    
    [[nodiscard]] auto SupportsTCP() const -> bool override { return true; }
    [[nodiscard]] auto SupportsUDP() const -> bool override { return true; }
    [[nodiscard]] auto SupportsIPv6() const -> bool override { return true; }
    [[nodiscard]] auto SupportsWebRTC() const -> bool override { return false; }

private:
    #ifdef _WIN32
    void InitializeWinsock();
    void CleanupWinsock();
    #endif
};

// WebAssembly platform implementation
class WebAssemblyPlatformNetworking : public PlatformNetworking {
public:
    [[nodiscard]] auto CreateTCPSocket() -> std::unique_ptr<NetworkConnection> override {
        // WebAssembly doesn't support raw TCP, use WebSocket instead
        return std::make_unique<WebSocketConnection>();
    }
    
    [[nodiscard]] auto CreateUDPSocket() -> std::unique_ptr<NetworkConnection> override {
        // Use WebRTC data channels for UDP-like functionality
        return std::make_unique<WebRTCConnection>();
    }
    
    [[nodiscard]] auto CreateServerSocket(const NetworkAddress& bind_address) -> std::unique_ptr<ServerSocket> override {
        // WebAssembly cannot create server sockets
        return nullptr;
    }
    
    [[nodiscard]] auto SupportsTCP() const -> bool override { return false; } // Only WebSocket
    [[nodiscard]] auto SupportsUDP() const -> bool override { return true; }  // Via WebRTC
    [[nodiscard]] auto SupportsIPv6() const -> bool override { return false; }
    [[nodiscard]] auto SupportsWebRTC() const -> bool override { return true; }
    
    // WebAssembly-specific functionality
    [[nodiscard]] auto CreateWebRTCConnection(const std::string& signaling_server) -> std::unique_ptr<NetworkConnection> {
        return std::make_unique<WebRTCConnection>(signaling_server);
    }
    
    [[nodiscard]] auto CreateWebSocketConnection() -> std::unique_ptr<NetworkConnection> {
        return std::make_unique<WebSocketConnection>();
    }
};

// Platform factory for automatic selection
class NetworkPlatformFactory {
public:
    [[nodiscard]] static auto CreatePlatformNetworking() -> std::unique_ptr<PlatformNetworking> {
        #ifdef __EMSCRIPTEN__
            return std::make_unique<WebAssemblyPlatformNetworking>();
        #else
            return std::make_unique<NativePlatformNetworking>();
        #endif
    }
    
    [[nodiscard]] static auto GetPlatformCapabilities() -> NetworkCapabilities {
        auto platform = CreatePlatformNetworking();
        return NetworkCapabilities{
            .supports_tcp = platform->SupportsTCP(),
            .supports_udp = platform->SupportsUDP(),
            .supports_ipv6 = platform->SupportsIPv6(),
            .supports_webrtc = platform->SupportsWebRTC(),
            .supports_server_sockets = platform->CreateServerSocket(NetworkAddress{"0.0.0.0", 0}) != nullptr
        };
    }
};

struct NetworkCapabilities {
    bool supports_tcp = false;
    bool supports_udp = false;
    bool supports_ipv6 = false;
    bool supports_webrtc = false;
    bool supports_server_sockets = false;
};

} // namespace engine::networking
```

### Headless Server Support

#### Dedicated Server Implementation

```cpp
namespace engine::networking {

// Headless server manager for dedicated server operation
class HeadlessServerManager {
public:
    struct ServerConfig {
        NetworkAddress bind_address{"0.0.0.0", 7777};
        size_t max_connections = 64;
        float tick_rate = 60.0f;                    // Server simulation rate
        float network_update_rate = 20.0f;          // Network update rate
        bool enable_logging = true;
        std::string log_file_path = "server.log";
        bool enable_console = true;                 // Console input for server commands
    };
    
    explicit HeadlessServerManager(const ServerConfig& config) : config_(config) {}
    
    [[nodiscard]] auto Initialize() -> bool {
        // Initialize engine systems without graphics, audio, or input
        if (!InitializeHeadlessEngine()) {
            return false;
        }
        
        // Create server topology
        server_topology_ = std::make_unique<ClientServerTopology>(true);
        if (!server_topology_->Initialize()) {
            return false;
        }
        
        // Start server listening
        if (!server_topology_->StartServer(config_.bind_address)) {
            return false;
        }
        
        // Initialize server systems
        InitializeServerSystems();
        
        LogMessage(LogLevel::Info, std::format("Headless server started on {}", config_.bind_address.ToString()));
        return true;
    }
    
    void Run() {
        is_running_ = true;
        auto last_tick = std::chrono::steady_clock::now();
        auto last_network_update = last_tick;
        
        const auto tick_interval = std::chrono::microseconds(static_cast<int64_t>(1000000.0f / config_.tick_rate));
        const auto network_interval = std::chrono::microseconds(static_cast<int64_t>(1000000.0f / config_.network_update_rate));
        
        while (is_running_) {
            auto current_time = std::chrono::steady_clock::now();
            
            // Process console input if enabled
            if (config_.enable_console) {
                ProcessConsoleInput();
            }
            
            // Server tick
            if (current_time - last_tick >= tick_interval) {
                auto delta_time = std::chrono::duration<float>(current_time - last_tick).count();
                UpdateServer(delta_time);
                last_tick = current_time;
            }
            
            // Network update
            if (current_time - last_network_update >= network_interval) {
                UpdateNetworking();
                last_network_update = current_time;
            }
            
            // Sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    void Shutdown() {
        is_running_ = false;
        
        if (server_topology_) {
            server_topology_->Shutdown();
        }
        
        ShutdownServerSystems();
        LogMessage(LogLevel::Info, "Headless server shut down");
    }
    
    // Server command interface
    void RegisterCommand(const std::string& command, std::function<void(const std::vector<std::string>&)> handler) {
        console_commands_[command] = handler;
    }
    
    // Server state queries
    [[nodiscard]] auto GetConnectedClients() const -> size_t {
        return server_topology_ ? server_topology_->GetConnections().size() : 0;
    }
    
    [[nodiscard]] auto GetServerUptime() const -> std::chrono::duration<float> {
        return std::chrono::steady_clock::now() - start_time_;
    }

private:
    ServerConfig config_;
    std::unique_ptr<ClientServerTopology> server_topology_;
    std::atomic<bool> is_running_{false};
    std::chrono::steady_clock::time_point start_time_;
    
    // Server systems (no graphics, audio, or input)
    std::unique_ptr<World> server_world_;
    std::unique_ptr<NetworkSystem> network_system_;
    
    // Console command system
    std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> console_commands_;
    
    [[nodiscard]] auto InitializeHeadlessEngine() -> bool {
        // Initialize only headless-compatible systems
        // Platform abstraction (file I/O, timing, memory)
        // ECS core
        // Threading system
        // Assets (for server-side data)
        // Physics (for server simulation)
        // Scripting (for server logic)
        // NO rendering, audio, or input systems
        
        server_world_ = std::make_unique<World>();
        return true;
    }
    
    void InitializeServerSystems() {
        // Create network system
        network_system_ = std::make_unique<NetworkSystem>(server_topology_);
        
        // Register server console commands
        RegisterDefaultCommands();
    }
    
    void ShutdownServerSystems() {
        network_system_.reset();
        server_world_.reset();
    }
    
    void UpdateServer(float delta_time) {
        // Update server simulation
        if (server_world_) {
            // Process ECS systems (physics, AI, game logic)
            // NO rendering, audio, or input processing
            server_world_->GetSystemScheduler().ExecuteFrame(delta_time);
        }
    }
    
    void UpdateNetworking() {
        // Process network messages and synchronization
        if (network_system_) {
            // Network system handles incoming/outgoing messages
            // Component synchronization
            // Authority management
        }
    }
    
    void ProcessConsoleInput() {
        // Check for console input without blocking
        // Parse and execute server commands
        static std::string input_buffer;
        
        // Platform-specific non-blocking console input
        #ifdef _WIN32
            if (_kbhit()) {
                char ch = _getch();
                if (ch == '\r' || ch == '\n') {
                    ProcessConsoleCommand(input_buffer);
                    input_buffer.clear();
                } else if (ch == '\b' && !input_buffer.empty()) {
                    input_buffer.pop_back();
                } else {
                    input_buffer += ch;
                }
            }
        #else
            // Linux non-blocking input implementation
        #endif
    }
    
    void ProcessConsoleCommand(const std::string& command_line) {
        auto args = SplitString(command_line, ' ');
        if (args.empty()) return;
        
        const auto& command = args[0];
        args.erase(args.begin());
        
        auto it = console_commands_.find(command);
        if (it != console_commands_.end()) {
            it->second(args);
        } else {
            LogMessage(LogLevel::Warning, std::format("Unknown command: {}", command));
        }
    }
    
    void RegisterDefaultCommands() {
        RegisterCommand("quit", [this](const std::vector<std::string>&) {
            is_running_ = false;
        });
        
        RegisterCommand("status", [this](const std::vector<std::string>&) {
            auto uptime = GetServerUptime();
            auto clients = GetConnectedClients();
            LogMessage(LogLevel::Info, std::format("Server uptime: {:.1f}s, Connected clients: {}", uptime.count(), clients));
        });
        
        RegisterCommand("kick", [this](const std::vector<std::string>& args) {
            if (args.empty()) {
                LogMessage(LogLevel::Warning, "Usage: kick <client_id>");
                return;
            }
            // Implementation for kicking clients
        });
    }
    
    [[nodiscard]] auto SplitString(const std::string& str, char delimiter) -> std::vector<std::string>;
};

} // namespace engine::networking
```

### Scripting Integration

#### Network Script API

```cpp
namespace engine::scripting {

// Network API for scripting languages
class ScriptNetworkAPI {
public:
    explicit ScriptNetworkAPI(NetworkSystem& network_system) 
        : network_system_(network_system) {}
    
    // Connection management
    [[nodiscard]] auto ConnectToServer(const std::string& address, uint16_t port) -> bool;
    [[nodiscard]] auto StartServer(uint16_t port) -> bool;
    void Disconnect();
    [[nodiscard]] auto IsConnected() const -> bool;
    [[nodiscard]] auto IsServer() const -> bool;
    
    // Message sending
    void SendMessage(const std::string& message_type, const ScriptValue& data);
    void SendMessageToConnection(ConnectionId connection_id, const std::string& message_type, const ScriptValue& data);
    void BroadcastMessage(const std::string& message_type, const ScriptValue& data);
    
    // Message handling
    void RegisterMessageHandler(const std::string& message_type, std::function<void(const ScriptValue&)> handler);
    void UnregisterMessageHandler(const std::string& message_type);
    
    // Entity networking
    void NetworkEntity(EntityId entity, bool has_authority = false);
    void UnnetworkEntity(EntityId entity);
    void SetEntityAuthority(EntityId entity, ConnectionId connection_id);
    [[nodiscard]] auto HasAuthority(EntityId entity) const -> bool;
    
    // Network statistics
    [[nodiscard]] auto GetNetworkStatistics() const -> ScriptValue;
    [[nodiscard]] auto GetConnectionCount() const -> uint32_t;
    [[nodiscard]] auto GetLatency(ConnectionId connection_id) const -> float;

private:
    NetworkSystem& network_system_;
    std::unordered_map<std::string, std::function<void(const ScriptValue&)>> message_handlers_;
};

// Integration with unified scripting interface
void RegisterNetworkAPI(ScriptInterface& script_interface, NetworkSystem& network_system) {
    auto network_api = std::make_unique<ScriptNetworkAPI>(network_system);
    script_interface.RegisterAPI("network", std::move(network_api));
}

} // namespace engine::scripting

// Example Lua integration
/*
-- Lua script example for networking
function on_client_connect()
    -- Connect to server
    if network.connect_to_server("localhost", 7777) then
        print("Connected to server")
        
        -- Register message handlers
        network.register_message_handler("player_joined", function(data)
            print("Player joined: " .. data.player_name)
        end)
        
        network.register_message_handler("game_state", function(data)
            -- Update local game state from server
            update_game_state(data)
        end)
    else
        print("Failed to connect to server")
    end
end

function on_player_move(entity_id, new_position)
    if network.has_authority(entity_id) then
        -- Send movement update to server
        network.send_message("player_move", {
            entity_id = entity_id,
            position = new_position
        })
    end
end

function on_server_start()
    if network.start_server(7777) then
        print("Server started on port 7777")
        
        -- Register server message handlers
        network.register_message_handler("player_move", function(data)
            -- Validate and broadcast movement
            if validate_movement(data.entity_id, data.position) then
                network.broadcast_message("player_move", data)
            end
        end)
    else
        print("Failed to start server")
    end
end
*/
```

## Performance Requirements

### Target Specifications

- **Networked Entities**: Support 100+ networked entities with 20Hz update rates
- **Latency**: Maintain sub-50ms average latency for typical gameplay scenarios
- **Bandwidth Efficiency**: Delta compression reducing bandwidth usage by 60-80%
- **Connection Capacity**: Handle 64 simultaneous connections on dedicated servers
- **Cross-Platform Performance**: <10% performance difference between native and WebAssembly networking
- **Memory Usage**: Network system overhead under 50MB for typical multiplayer scenarios

### Optimization Strategies

1. **Delta Compression**: Only transmit changed component data to minimize bandwidth
2. **Message Batching**: Combine multiple small messages into larger packets
3. **Priority-Based Updates**: More frequent updates for important entities (players vs. background objects)
4. **Spatial Culling**: Only sync entities relevant to each client's area of interest
5. **Adaptive Quality**: Reduce update frequency and precision based on network conditions

## Integration Points

### ECS Core Integration

- Networked components integrated seamlessly with existing ECS architecture
- Network system scheduled appropriately in ECS system execution order
- Component replication uses standard ECS component queries and updates

### Platform Abstraction Integration

- Socket implementation uses platform abstraction for cross-platform compatibility
- File I/O integration for configuration and logging
- High-resolution timing for network performance measurement

### Threading System Integration

- Network I/O operations executed on dedicated network thread pool
- Message processing integrated with job system for parallel execution
- Thread-safe network state management compatible with ECS threading model

### Scripting System Integration

- Complete network API exposed to Lua, Python, and AngelScript
- Network event handling scriptable for custom game protocols
- Message serialization supports script-defined data structures

This networking system provides comprehensive multiplayer foundation while maintaining flexibility to support diverse
networking architectures. The headless server support enables dedicated server deployment, while the cross-platform
design ensures consistent behavior across all target platforms.
