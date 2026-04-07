// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <string>
#include "player_input.hpp"

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
};


