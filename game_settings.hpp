// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <string>
#include "player_input.hpp"

struct PlayerProfile
{
    std::string nickname = "Player";

    int matches_played = 0;
    int total_kills = 0;
    int total_deaths = 0;
    int best_match_kills = 0;
};

struct GameSettings
{
    enum class Team
    {
        Fire,
        Water,
        Spectator
    };

    enum class NetworkRole
    {
        None,
        Host,
        Client
    };

    PlayerKeybinds local_keys
    {
        sf::Keyboard::Scancode::W,
        sf::Keyboard::Scancode::S,
        sf::Keyboard::Scancode::A,
        sf::Keyboard::Scancode::D,
        sf::Keyboard::Scancode::J,
        sf::Keyboard::Scancode::K
    };

    std::string nickname = "Player";

    Team chosen_team = Team::Spectator;

	NetworkRole network_role = NetworkRole::None;

    std::string server_ip = "127.0.0.1";

    unsigned short server_port = 53000;

    int latest_fire_count = 0;
    int latest_water_count = 0;
    int latest_spectator_count = 0;
    int team_limit = 10;

    Team last_winner_team = Team::Spectator;

    PlayerProfile profile;
};


