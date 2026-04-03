// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "client_session.hpp"

ClientSession::ClientSession(NetworkManager& network)
    : m_network(&network)
{
}

bool ClientSession::connect(const sf::IpAddress& ip, unsigned short port)
{
    if (!m_network)
        return false;

    return m_network->start_client(ip, port);
}

bool ClientSession::is_connected() const
{
    if (!m_network)
        return false;

    return m_network->is_connected();
}

bool ClientSession::send_local_input(const PlayerInput& input)
{
    if (!m_network)
        return false;

    return m_network->send_input(input);
}

std::optional<WorldStatePacket> ClientSession::poll_world_state()
{
    if (!m_network)
        return std::nullopt;

    return m_network->receive_world_state();
}