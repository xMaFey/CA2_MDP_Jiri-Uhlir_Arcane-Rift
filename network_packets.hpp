// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "player_input.hpp"
#include <SFML/System/Vector2.hpp>
#include <vector>

enum class PacketType
{
    PlayerInput = 1,
    WorldState = 2
};

struct PlayerInputPacket
{
    PlayerInput input;
};

struct BulletState
{
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f dir{ 1.f, 0.f };
    int owner = 0;
    int spell = 0; // 0 = fire, 1 = water
};

struct WorldStatePacket
{
    sf::Vector2f p1_pos{ 0.f, 0.f };
    sf::Vector2f p2_pos{ 0.f, 0.f };

    sf::Vector2f p1_dir{ 1.f, 0.f };
    sf::Vector2f p2_dir{ 1.f, 0.f };

    int fire_kills = 0;
    int water_kills = 0;

    std::vector<BulletState> bullets;
};