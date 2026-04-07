// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================


#pragma once

#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Text.hpp>
#include <string>

class TeamSelectState : public State
{
public:
    TeamSelectState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    void refresh_text();

private:
    sf::Text m_title;
    sf::Text m_mode_text;
    sf::Text m_name_text;
    sf::Text m_fire_text;
    sf::Text m_water_text;
    sf::Text m_hint;

    gui::Container m_gui;

    std::string m_nickname = "Player";

    int m_fire_count = 0;
    int m_water_count = 0;
    int m_team_limit = 10;
};
