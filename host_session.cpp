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

std::optional<JoinInfoPacket> HostSession::poll_join_info()
{
    if (!m_network)
        return std::nullopt;

    return m_network->receive_join_info();
}

std::optional<std::pair<int, PlayerInput>> HostSession::poll_remote_input()
{
    if (!m_network)
        return std::nullopt;

    return m_network->receive_input();
}

std::optional<std::pair<int, TeamChangeRequestPacket>> HostSession::poll_team_change_request()
{
    if (!m_network)
        return std::nullopt;

    return m_network->receive_team_change_request();
}

bool HostSession::send_world_state_to_player(int player_id, const WorldStatePacket& state)
{
    if (!m_network)
        return false;

    return m_network->send_world_state_to_player(player_id, state);
}

bool HostSession::send_world_state_to_all(const WorldStatePacket& state)
{
    if (!m_network)
        return false;

    return m_network->send_world_state_to_all(state);
}

bool HostSession::send_lobby_state_to_player(int player_id, const LobbyStatePacket& state)
{
    if (!m_network)
        return false;

    return m_network->send_lobby_state_to_player(player_id, state);
}

bool HostSession::send_lobby_state_to_all(const LobbyStatePacket& state)
{
    if (!m_network)
        return false;

    return m_network->send_lobby_state_to_all(state);
}

std::vector<int> HostSession::consume_disconnected_player_ids()
{
    if (!m_network)
        return {};

    return m_network->consume_disconnected_player_ids();
}