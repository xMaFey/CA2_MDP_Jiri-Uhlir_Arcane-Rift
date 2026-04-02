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
    m_socket.setBlocking(false);

    if (m_socket.connect(ip, port, sf::seconds(2.f)) != sf::Socket::Status::Done)
        return false;

    m_connected = true;
    m_socket.setBlocking(false);
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