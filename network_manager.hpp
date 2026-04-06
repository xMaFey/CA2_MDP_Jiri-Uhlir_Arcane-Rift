// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <SFML/Network.hpp>
#include <optional>
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

    // join packet
    bool send_join_info(const JoinInfoPacket& joinInfo);
    std::optional<JoinInfoPacket> receive_join_info();

    // input packet
    bool send_input(const PlayerInput& input);
    std::optional<PlayerInput> receive_input();

    // world state packet
    bool send_world_state(const WorldStatePacket& state);
    std::optional<WorldStatePacket> receive_world_state();

private:
    Mode m_mode = Mode::None;

    sf::TcpListener m_listener;
    sf::TcpSocket m_socket;
    bool m_connected = false;
};