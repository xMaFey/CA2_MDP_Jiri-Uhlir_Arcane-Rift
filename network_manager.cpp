// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "network_manager.hpp"
#include <algorithm>

namespace
{
    // serializing join packet
    sf::Packet& operator<<(sf::Packet& packet, const JoinInfoPacket& joinInfo)
    {
        return packet << joinInfo.player_id << joinInfo.nickname << joinInfo.team;
    }

    sf::Packet& operator>>(sf::Packet& packet, JoinInfoPacket& joinInfo)
    {
        return packet >> joinInfo.player_id >> joinInfo.nickname >> joinInfo.team;
    }

    // serializing team change request packet
    sf::Packet& operator<<(sf::Packet& packet, const TeamChangeRequestPacket& request)
    {
        return packet << request.requested_team;
    }

    sf::Packet& operator>>(sf::Packet& packet, TeamChangeRequestPacket& request)
    {
        return packet >> request.requested_team;
    }

    // serializing lobby player state
    sf::Packet& operator<<(sf::Packet& packet, const LobbyPlayerState& p)
    {
        return packet
            << p.id
            << p.nickname
            << p.team
            << p.connected;
    }

    sf::Packet& operator>>(sf::Packet& packet, LobbyPlayerState& p)
    {
        return packet
            >> p.id
            >> p.nickname
            >> p.team
            >> p.connected;
    }

    // serializing lobby state
    sf::Packet& operator<<(sf::Packet& packet, const LobbyStatePacket& state)
    {
        packet
            << state.your_player_id
            << state.fire_count
            << state.water_count
            << state.spectator_count
            << state.can_join_fire
            << state.can_join_water
            << state.can_spectate
            << state.match_started
            << static_cast<uint32_t>(state.players.size());

        for (const auto& p : state.players)
            packet << p;

        return packet;
    }

    sf::Packet& operator>>(sf::Packet& packet, LobbyStatePacket& state)
    {
        uint32_t playerCount = 0;

        packet
            >> state.your_player_id
            >> state.fire_count
            >> state.water_count
            >> state.spectator_count
            >> state.can_join_fire
            >> state.can_join_water
            >> state.can_spectate
            >> state.match_started
            >> playerCount;

        state.players.clear();
        state.players.reserve(playerCount);

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            LobbyPlayerState p;
            packet >> p;
            state.players.push_back(p);
        }

        return packet;
    }

    // serializing player net state
    sf::Packet& operator<<(sf::Packet& packet, const PlayerNetState& p)
    {
        return packet
            << p.id
            << p.nickname
            << p.pos.x << p.pos.y
            << p.dir.x << p.dir.y
            << p.team
            << p.connected
            << p.has_pending_team_change
            << p.pending_team
            << p.anim_state;
    }

    sf::Packet& operator>>(sf::Packet& packet, PlayerNetState& p)
    {
        return packet
            >> p.id
            >> p.nickname
            >> p.pos.x >> p.pos.y
            >> p.dir.x >> p.dir.y
            >> p.team
            >> p.connected
            >> p.has_pending_team_change
            >> p.pending_team
            >> p.anim_state;
    }

    sf::Packet& operator<<(sf::Packet& packet, const SoundEventState& s)
    {
        return packet << s.sound_id << s.source_player_id;
    }

    sf::Packet& operator>>(sf::Packet& packet, SoundEventState& s)
    {
        return packet >> s.sound_id >> s.source_player_id;
    }

    // serializing player input
    sf::Packet& operator<<(sf::Packet& packet, const PlayerInput& input)
    {
        return packet << input.move.x << input.move.y << input.shootHeld << input.dashPressed;
    }

    sf::Packet& operator>>(sf::Packet& packet, PlayerInput& input)
    {
        return packet >> input.move.x >> input.move.y >> input.shootHeld >> input.dashPressed;
    }

    // serializing bullet state
    sf::Packet& operator<<(sf::Packet& packet, const BulletState& b)
    {
        return packet
            << b.bullet_id
            << b.pos.x << b.pos.y
            << b.dir.x << b.dir.y
            << b.owner
            << b.spell;
    }

    sf::Packet& operator>>(sf::Packet& packet, BulletState& b)
    {
        return packet
            >> b.bullet_id
            >> b.pos.x >> b.pos.y
            >> b.dir.x >> b.dir.y
            >> b.owner
            >> b.spell;
    }

    // serializing world state
    sf::Packet& operator<<(sf::Packet& packet, const WorldStatePacket& state)
    {
        packet
            << state.your_player_id
            << static_cast<uint32_t>(state.players.size())
            << state.fire_count
            << state.water_count
            << state.spectator_count
            << state.fire_kills
            << state.water_kills;

        for (const auto& p : state.players)
            packet << p;

        packet << static_cast<uint32_t>(state.bullets.size());

        for (const auto& b : state.bullets)
            packet << b;

        // send sound events too
        packet << static_cast<uint32_t>(state.sound_events.size());

        for (const auto& s : state.sound_events)
            packet << s;

        return packet;
    }

    sf::Packet& operator>>(sf::Packet& packet, WorldStatePacket& state)
    {
        uint32_t playerCount = 0;
        packet >> state.your_player_id
            >> playerCount
            >> state.fire_count
            >> state.water_count
            >> state.spectator_count
            >> state.fire_kills
            >> state.water_kills;

        state.players.clear();
        state.players.reserve(playerCount);

        for (uint32_t i = 0; i < playerCount; ++i)
        {
            PlayerNetState p;
            packet >> p;
            state.players.push_back(p);
        }

        uint32_t bulletCount = 0;
        packet >> bulletCount;

        state.bullets.clear();
        state.bullets.reserve(bulletCount);

        for (uint32_t i = 0; i < bulletCount; ++i)
        {
            BulletState b;
            packet >> b;
            state.bullets.push_back(b);
        }

        // receive sound events too
        uint32_t soundCount = 0;
        packet >> soundCount;

        state.sound_events.clear();
        state.sound_events.reserve(soundCount);

        for (uint32_t i = 0; i < soundCount; ++i)
        {
            SoundEventState s;
            packet >> s;
            state.sound_events.push_back(s);
        }

        return packet;
    }

    // packet type helpers
    sf::Packet make_typed_packet(PacketType type)
    {
        sf::Packet packet;
        packet << static_cast<int>(type);
        return packet;
    }

    std::optional<PacketType> read_packet_type(sf::Packet& packet)
    {
        int typeValue = 0;
        if (!(packet >> typeValue))
            return std::nullopt;

        return static_cast<PacketType>(typeValue);
    }
}

bool NetworkManager::start_host(unsigned short port)
{
    disconnect();

    m_mode = Mode::Host;
    m_next_player_id = 1;

    m_listener.setBlocking(false);
    if (m_listener.listen(port) != sf::Socket::Status::Done)
        return false;

    m_connected = true; // host is active even before clients join
    return true;
}

bool NetworkManager::start_client(const sf::IpAddress& ip, unsigned short port)
{
    disconnect();

    m_mode = Mode::Client;

    m_client_socket.setBlocking(true);

    if (m_client_socket.connect(ip, port, sf::seconds(2.f)) != sf::Socket::Status::Done)
    {
        m_connected = false;
        return false;
    }

    m_connected = true;
    m_client_socket.setBlocking(false);
    return true;
}

void NetworkManager::disconnect()
{
    m_listener.close();

    for (auto& client : m_host_clients)
    {
        if (client.socket)
            client.socket->disconnect();
    }

    m_host_clients.clear();

    m_client_socket.disconnect();

    m_connected = false;
    m_mode = Mode::None;
    m_next_player_id = 1;
    m_disconnected_player_ids.clear();
}

bool NetworkManager::is_connected() const
{
    return m_connected;
}

NetworkManager::Mode NetworkManager::mode() const
{
    return m_mode;
}

std::vector<int> NetworkManager::consume_disconnected_player_ids()
{
    std::vector<int> out = std::move(m_disconnected_player_ids);
    m_disconnected_player_ids.clear();
    return out;
}

void NetworkManager::accept_new_clients()
{
    if (m_mode != Mode::Host)
        return;

    while (true)
    {
        auto socket = std::make_unique<sf::TcpSocket>();
        socket->setBlocking(false);

        const auto status = m_listener.accept(*socket);
        if (status != sf::Socket::Status::Done)
            break;

        HostClient client;
        client.socket = std::move(socket);
        client.player_id = -1;

        m_host_clients.push_back(std::move(client));
    }
}

void NetworkManager::poll_host_client_packets()
{
    if (m_mode != Mode::Host)
        return;

    accept_new_clients();

    for (auto& client : m_host_clients)
    {
        if (!client.socket)
            continue;

        while (true)
        {
            sf::Packet packet;

            const auto status = client.socket->receive(packet);

            if (status == sf::Socket::Status::Disconnected)
            {
                if (client.player_id >= 0)
                    m_disconnected_player_ids.push_back(client.player_id);

                client.socket->disconnect();
                client.socket.reset();
                break;
            }

            if (status != sf::Socket::Status::Done)
                break;

            const auto type = read_packet_type(packet);
            if (!type.has_value())
                continue;

            if (*type == PacketType::JoinInfo)
            {
                JoinInfoPacket joinInfo;
                packet >> joinInfo;

                if (client.player_id < 0)
                    client.player_id = m_next_player_id++;

                joinInfo.player_id = client.player_id;
                client.pending_join_info = joinInfo;
            }
            else if (*type == PacketType::PlayerInput)
            {
                PlayerInput input;
                packet >> input;
                client.pending_input = input;
            }
            else if (*type == PacketType::TeamChangeRequest)
            {
                TeamChangeRequestPacket request;
                packet >> request;
                client.pending_team_change_request = request;
            }
        }
    }

    m_host_clients.erase(
        std::remove_if(
            m_host_clients.begin(),
            m_host_clients.end(),
            [](const HostClient& client)
            {
                return !client.socket;
            }),
        m_host_clients.end()
    );
}

bool NetworkManager::send_join_info(const JoinInfoPacket& joinInfo)
{
    if (m_mode != Mode::Client || !m_connected)
        return false;

    sf::Packet packet = make_typed_packet(PacketType::JoinInfo);
    packet << joinInfo;

    return m_client_socket.send(packet) == sf::Socket::Status::Done;
}

std::optional<JoinInfoPacket> NetworkManager::receive_join_info()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_host_client_packets();

    for (auto& client : m_host_clients)
    {
        if (!client.pending_join_info.has_value())
            continue;

        JoinInfoPacket joinInfo = *client.pending_join_info;
        client.pending_join_info.reset();
        return joinInfo;
    }

    return std::nullopt;
}

bool NetworkManager::send_input(const PlayerInput& input)
{
    if (m_mode != Mode::Client || !m_connected)
        return false;

    sf::Packet packet = make_typed_packet(PacketType::PlayerInput);
    packet << input;

    return m_client_socket.send(packet) == sf::Socket::Status::Done;
}

bool NetworkManager::send_team_change_request(const TeamChangeRequestPacket& request)
{
    if (m_mode != Mode::Client || !m_connected)
        return false;

    sf::Packet packet = make_typed_packet(PacketType::TeamChangeRequest);
    packet << request;

    return m_client_socket.send(packet) == sf::Socket::Status::Done;
}

std::optional<std::pair<int, PlayerInput>> NetworkManager::receive_input()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_host_client_packets();

    for (auto& client : m_host_clients)
    {
        if (client.player_id < 0 || !client.pending_input.has_value())
            continue;

        PlayerInput input = *client.pending_input;
        client.pending_input.reset();
        return std::make_pair(client.player_id, input);
    }

    return std::nullopt;
}

std::optional<std::pair<int, TeamChangeRequestPacket>> NetworkManager::receive_team_change_request()
{
    if (m_mode != Mode::Host)
        return std::nullopt;

    poll_host_client_packets();

    for (auto& client : m_host_clients)
    {
        if (client.player_id < 0 || !client.pending_team_change_request.has_value())
            continue;

        TeamChangeRequestPacket request = *client.pending_team_change_request;
        client.pending_team_change_request.reset();
        return std::make_pair(client.player_id, request);
    }

    return std::nullopt;
}

bool NetworkManager::send_lobby_state_to_player(int player_id, const LobbyStatePacket& state)
{
    if (m_mode == Mode::Client || !m_connected)
        return false;

    accept_new_clients();

    for (auto& client : m_host_clients)
    {
        if (!client.socket)
            continue;

        if (client.player_id != player_id)
            continue;

        sf::Packet packet = make_typed_packet(PacketType::LobbyState);
        packet << state;

        return client.socket->send(packet) == sf::Socket::Status::Done;
    }

    return false;
}

bool NetworkManager::send_lobby_state_to_all(const LobbyStatePacket& state)
{
    if (m_mode == Mode::Client || !m_connected)
        return false;

    accept_new_clients();

    bool sentAny = false;

    for (auto& client : m_host_clients)
    {
        if (!client.socket)
            continue;

        sf::Packet packet = make_typed_packet(PacketType::LobbyState);
        packet << state;

        if (client.socket->send(packet) == sf::Socket::Status::Done)
            sentAny = true;
    }

    return sentAny;
}

std::optional<LobbyStatePacket> NetworkManager::receive_lobby_state()
{
    if (m_mode != Mode::Client || !m_connected)
        return std::nullopt;

    sf::Packet packet;
    const auto status = m_client_socket.receive(packet);

    if (status != sf::Socket::Status::Done)
        return std::nullopt;

    const auto type = read_packet_type(packet);
    if (!type.has_value() || *type != PacketType::LobbyState)
        return std::nullopt;

    LobbyStatePacket state;
    packet >> state;
    return state;
}

bool NetworkManager::send_world_state_to_player(int player_id, const WorldStatePacket& state)
{
    if (m_mode == Mode::Client || !m_connected)
        return false;

    accept_new_clients();

    for (auto& client : m_host_clients)
    {
        if (!client.socket)
            continue;

        if (client.player_id != player_id)
            continue;

        sf::Packet packet = make_typed_packet(PacketType::WorldState);
        packet << state;

        return client.socket->send(packet) == sf::Socket::Status::Done;
    }

    return false;
}

bool NetworkManager::send_world_state_to_all(const WorldStatePacket& state)
{
    if (m_mode == Mode::Client || !m_connected)
        return false;

    accept_new_clients();

    bool sentAny = false;

    for (auto& client : m_host_clients)
    {
        if (!client.socket)
            continue;

        sf::Packet packet = make_typed_packet(PacketType::WorldState);
        packet << state;

        if (client.socket->send(packet) == sf::Socket::Status::Done)
            sentAny = true;
    }

    return sentAny;
}

std::optional<WorldStatePacket> NetworkManager::receive_world_state()
{
    if (m_mode != Mode::Client || !m_connected)
        return std::nullopt;

    sf::Packet packet;
    const auto status = m_client_socket.receive(packet);

    if (status != sf::Socket::Status::Done)
        return std::nullopt;

    const auto type = read_packet_type(packet);
    if (!type.has_value() || *type != PacketType::WorldState)
        return std::nullopt;

    WorldStatePacket state;
    packet >> state;
    return state;
}