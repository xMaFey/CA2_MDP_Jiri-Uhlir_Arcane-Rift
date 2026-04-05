// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "network_manager.hpp"

namespace
{
    sf::Packet& operator<<(sf::Packet& packet, const PlayerInput& input)
    {
        return packet << input.move.x << input.move.y << input.shootHeld << input.dashPressed;
    }

    sf::Packet& operator>>(sf::Packet& packet, PlayerInput& input)
    {
        return packet >> input.move.x >> input.move.y >> input.shootHeld >> input.dashPressed;
    }

    sf::Packet& operator<<(sf::Packet& packet, const BulletState& b)
    {
        return packet
            << b.pos.x << b.pos.y
            << b.dir.x << b.dir.y
            << b.owner
            << b.spell;
    }

    sf::Packet& operator>>(sf::Packet& packet, BulletState& b)
    {
        return packet
            >> b.pos.x >> b.pos.y
            >> b.dir.x >> b.dir.y
            >> b.owner
            >> b.spell;
    }

    sf::Packet& operator<<(sf::Packet& packet, const WorldStatePacket& state)
    {
        packet
            << state.p1_pos.x << state.p1_pos.y
            << state.p2_pos.x << state.p2_pos.y
            << state.p1_dir.x << state.p1_dir.y
            << state.p2_dir.x << state.p2_dir.y
            << state.fire_kills << state.water_kills;

        packet << static_cast<uint32_t>(state.bullets.size());

        for (const auto& b : state.bullets)
            packet << b;

        return packet;
    }

    sf::Packet& operator>>(sf::Packet& packet, WorldStatePacket& state)
    {
        packet
            >> state.p1_pos.x >> state.p1_pos.y
            >> state.p2_pos.x >> state.p2_pos.y
            >> state.p1_dir.x >> state.p1_dir.y
            >> state.p2_dir.x >> state.p2_dir.y
            >> state.fire_kills >> state.water_kills;

        uint32_t count = 0;
        packet >> count;

        state.bullets.clear();
        state.bullets.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            BulletState b;
            packet >> b;
            state.bullets.push_back(b);
        }

        return packet;
    }
}

bool NetworkManager::start_host(unsigned short port)
{
    disconnect();

    m_mode = Mode::Host;

    m_listener.setBlocking(false);
    if (m_listener.listen(port) != sf::Socket::Status::Done)
        return false;

    m_socket.setBlocking(false);
    return true;
}

bool NetworkManager::start_client(const sf::IpAddress& ip, unsigned short port)
{
    disconnect();

    m_mode = Mode::Client;

    m_socket.setBlocking(true); // IMPORTANT: blocking during connect

    if (m_socket.connect(ip, port, sf::seconds(2.f)) != sf::Socket::Status::Done)
    {
        m_connected = false;
        return false;
    }
    
    m_connected = true;
    m_socket.setBlocking(false); // switch to non-blocking after successful connect
    return true;
}

void NetworkManager::disconnect()
{
    m_listener.close();
    m_socket.disconnect();
    m_connected = false;
    m_mode = Mode::None;
}

bool NetworkManager::is_connected() const
{
    return m_connected;
}

NetworkManager::Mode NetworkManager::mode() const
{
    return m_mode;
}

bool NetworkManager::send_input(const PlayerInput& input)
{
    if (!m_connected)
        return false;

    sf::Packet packet;
    packet << input;

    return m_socket.send(packet) == sf::Socket::Status::Done;
}

std::optional<PlayerInput> NetworkManager::receive_input()
{
    if (m_mode == Mode::Host && !m_connected)
    {
        if (m_listener.accept(m_socket) == sf::Socket::Status::Done)
        {
            m_connected = true;
            m_socket.setBlocking(false);
        }
    }

    if (!m_connected)
        return std::nullopt;

    sf::Packet packet;
    const auto status = m_socket.receive(packet);

    if (status == sf::Socket::Status::Done)
    {
        PlayerInput input;
        packet >> input;
        return input;
    }

    return std::nullopt;
}

bool NetworkManager::send_world_state(const WorldStatePacket& state)
{
    if (!m_connected)
        return false;

    sf::Packet packet;
    packet << state;

    return m_socket.send(packet) == sf::Socket::Status::Done;
}

std::optional<WorldStatePacket> NetworkManager::receive_world_state()
{
    if (!m_connected)
        return std::nullopt;

    sf::Packet packet;
    const auto status = m_socket.receive(packet);

    if (status == sf::Socket::Status::Done)
    {
        WorldStatePacket state;
        packet >> state;
        return state;
    }

    return std::nullopt;
}