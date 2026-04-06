// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "network_manager.hpp"
#include <optional>
#include "network_packets.hpp"

class HostSession
{
public:
    explicit HostSession(NetworkManager& network);

    bool start(unsigned short port);
    bool is_connected() const;

    // join packet from client
    std::optional<JoinInfoPacket> poll_join_info();

    // gameplay input from client
    std::optional<PlayerInput> poll_remote_input();

    // world state sent to client
    bool send_world_state(const WorldStatePacket& state);

private:
    NetworkManager* m_network = nullptr;
};