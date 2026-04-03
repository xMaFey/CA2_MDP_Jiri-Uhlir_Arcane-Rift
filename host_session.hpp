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

    std::optional<PlayerInput> poll_remote_input();
    bool send_world_state(const WorldStatePacket& state);

private:
    NetworkManager* m_network = nullptr;
};