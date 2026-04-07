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
    WorldState = 3,
    TeamChangeRequest = 4
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
    int player_id = -1;
    std::string nickname = "Player";
    int team = static_cast<int>(NetTeam::Spectator);
};

// sent by client to host when the player wants to switch team
struct TeamChangeRequestPacket
{
    int requested_team = static_cast<int>(NetTeam::Spectator);
};

struct PlayerNetState
{
    int id = -1;
    std::string nickname = "Player";
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f dir{ 1.f, 0.f };
    int team = static_cast<int>(NetTeam::Spectator);
    bool connected = false;

    // Pending team switch info, sent by host so clients can show it in HUD.
    bool has_pending_team_change = false;
    int pending_team = static_cast<int>(NetTeam::Spectator);
};

struct BulletState
{
    sf::Vector2f pos{ 0.f, 0.f };
    sf::Vector2f dir{ 1.f, 0.f };
    int owner = 0;      // player id of the bullet owner
    int spell = 0;      // 0 = fire, 1 = water
};

struct WorldStatePacket
{
    int your_player_id = -1; // tells each client which player slot is theirs

    std::vector<PlayerNetState> players;

    int fire_count = 0;
    int water_count = 0;
    int spectator_count = 0;

    int fire_kills = 0;
    int water_kills = 0;

    std::vector<BulletState> bullets;
};