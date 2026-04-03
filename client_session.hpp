// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "network_manager.hpp"
#include <SFML/Network/IpAddress.hpp>
#include "network_packets.hpp"
#include <optional>

class ClientSession
{
public:
    explicit ClientSession(NetworkManager& network);

    bool connect(const sf::IpAddress& ip, unsigned short port);
    bool is_connected() const;

    bool send_local_input(const PlayerInput& input);
    std::optional<WorldStatePacket> poll_world_state();

private:
    NetworkManager* m_network = nullptr;
};