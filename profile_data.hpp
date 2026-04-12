// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "game_settings.hpp"
#include <string>

namespace ProfileData
{
    // File used for local persistent player data on this PC.
    inline const std::string kProfileFile = "player_profile.txt";

    bool Load(PlayerProfile& profile);
    bool Save(const PlayerProfile& profile);
}

