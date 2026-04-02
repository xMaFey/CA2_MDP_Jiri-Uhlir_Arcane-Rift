// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <SFML/Network.hpp>
#include <optional>
#include "player_input.hpp"

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

    bool send_input(const PlayerInput& input);
    std::optional<PlayerInput> receive_input();

private:
    Mode m_mode = Mode::None;

    sf::TcpListener m_listener;
    sf::TcpSocket m_socket;
    bool m_connected = false;
};