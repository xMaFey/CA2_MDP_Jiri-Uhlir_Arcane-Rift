// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "host_session.hpp"

HostSession::HostSession(NetworkManager& network)
    : m_network(&network)
{
}

bool HostSession::start(unsigned short port)
{
    if (!m_network)
        return false;

    return m_network->start_host(port);
}

bool HostSession::is_connected() const
{
    if (!m_network)
        return false;

    return m_network->is_connected();
}

std::optional<PlayerInput> HostSession::poll_remote_input()
{
    if (!m_network)
        return std::nullopt;

    return m_network->receive_input();
}

bool HostSession::send_world_state(const WorldStatePacket& state)
{
    if (!m_network)
        return false;

    return m_network->send_world_state(state);
}