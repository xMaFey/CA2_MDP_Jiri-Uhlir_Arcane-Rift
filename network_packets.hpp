// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "player_input.hpp"
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <string>

enum class PacketType
{
    JoinInfo = 1,
    PlayerInput = 2,
    WorldState = 3
};

enum class NetTeam
{
    Fire = 0,
    Water = 1,
    Spectator = 2
};

// sent once by client after connecting
struct JoinInfoPacket
{
    std::string nickname = "Player";
    int team = static_cast<int>(NetTeam::Spectator);
};

struct BulletState
{
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f dir{ 1.f, 0.f };
    int owner = 0;      // 1 = host side, 2 = client side
    int spell = 0;      // 0 = fire, 1 = water
};

struct WorldStatePacket
{
    sf::Vector2f p1_pos{ 0.f, 0.f };
    sf::Vector2f p2_pos{ 0.f, 0.f };

    sf::Vector2f p1_dir{ 1.f, 0.f };
    sf::Vector2f p2_dir{ 1.f, 0.f };

    int p1_team = static_cast<int>(NetTeam::Spectator);
    int p2_team = static_cast<int>(NetTeam::Spectator);

    int fire_kills = 0;
    int water_kills = 0;

    std::vector<BulletState> bullets;
};