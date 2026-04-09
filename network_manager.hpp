// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <SFML/Network.hpp>
#include <optional>
#include <memory>
#include <vector>
#include "player_input.hpp"
#include "network_packets.hpp"

class NetworkManager
{
public:
    enum class Mode
    {
        None,
        Host,
        Client
    };

public:
    bool start_host(unsigned short port);
    bool start_client(const sf::IpAddress& ip, unsigned short port);
    void disconnect();

    bool is_connected() const;
    Mode mode() const;

    // client -> host
    bool send_join_info(const JoinInfoPacket& joinInfo);
    std::optional<JoinInfoPacket> receive_join_info();

    std::optional<LobbyStatePacket> receive_lobby_state();

    bool send_input(const PlayerInput& input);
    std::optional<std::pair<int, PlayerInput>> receive_input();

    // client -> host team switch request
    bool send_team_change_request(const TeamChangeRequestPacket& request);
    std::optional<std::pair<int, TeamChangeRequestPacket>> receive_team_change_request();

    // host -> client(s)
    bool send_world_state_to_player(int player_id, const WorldStatePacket& state);
    bool send_world_state_to_all(const WorldStatePacket& state);
    std::optional<WorldStatePacket> receive_world_state();

    bool send_lobby_state_to_player(int player_id, const LobbyStatePacket& state);
    bool send_lobby_state_to_all(const LobbyStatePacket& state);

    // disconnect tracking
    std::vector<int> consume_disconnected_player_ids();

private:
    void accept_new_clients();
    void poll_host_client_packets();

private:
    struct HostClient
    {
        std::unique_ptr<sf::TcpSocket> socket;
        int player_id = -1;

        std::optional<JoinInfoPacket> pending_join_info;
        std::optional<PlayerInput> pending_input;
        std::optional<TeamChangeRequestPacket> pending_team_change_request;
    };

private:
    Mode m_mode = Mode::None;

    sf::TcpListener m_listener;
    sf::TcpSocket m_client_socket;

    std::vector<HostClient> m_host_clients;
    std::vector<int> m_disconnected_player_ids;

    bool m_connected = false;
    int m_next_player_id = 1;
};