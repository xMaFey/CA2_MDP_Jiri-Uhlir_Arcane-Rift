// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "profile_data.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace
{
    std::string trim(const std::string& s)
    {
        const auto first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return "";

        const auto last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    }

    bool parse_int(const std::string& text, int& outValue)
    {
        std::istringstream ss(text);
        ss >> outValue;
        return !ss.fail();
    }
}

bool ProfileData::Load(PlayerProfile& profile)
{
    std::ifstream file(kProfileFile);
    if (!file.is_open())
        return false;

    PlayerProfile loaded;

    std::string line;
    while (std::getline(file, line))
    {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (key == "nickname")
        {
            loaded.nickname = value.empty() ? "Player" : value;
        }
        else if (key == "matches_played")
        {
            parse_int(value, loaded.matches_played);
        }
        else if (key == "total_kills")
        {
            parse_int(value, loaded.total_kills);
        }
        else if (key == "total_deaths")
        {
            parse_int(value, loaded.total_deaths);
        }
        else if (key == "best_match_kills")
        {
            parse_int(value, loaded.best_match_kills);
        }
    }

    if (loaded.nickname.empty())
        loaded.nickname = "Player";

    profile = loaded;
    return true;
}

bool ProfileData::Save(const PlayerProfile& profile)
{
    std::ofstream file(kProfileFile, std::ios::trunc);
    if (!file.is_open())
        return false;

    // Simple text format so it is easy to inspect and debug.
    file << "nickname=" << profile.nickname << "\n";
    file << "matches_played=" << profile.matches_played << "\n";
    file << "total_kills=" << profile.total_kills << "\n";
    file << "total_deaths=" << profile.total_deaths << "\n";
    file << "best_match_kills=" << profile.best_match_kills << "\n";

    return true;
}