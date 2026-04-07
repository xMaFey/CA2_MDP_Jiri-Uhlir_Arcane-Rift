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
    std::optional<std::pair<int, PlayerInput>> poll_remote_input();

    // team switch request from client
    std::optional<std::pair<int, TeamChangeRequestPacket>> poll_team_change_request();

    // world state sent to client
    bool send_world_state_to_player(int player_id, const WorldStatePacket& state);
    bool send_world_state_to_all(const WorldStatePacket& state);

    std::vector<int> consume_disconnected_player_ids();

private:
    NetworkManager* m_network = nullptr;
};