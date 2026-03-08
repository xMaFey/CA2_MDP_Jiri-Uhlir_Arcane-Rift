// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
// Gameplay state (currently just moving a circle).
// Esc -> Pause_state

#include <SFML/Graphics.hpp>
#include "state.hpp"

class Play_state : public State
{
public:
    explicit Play_state(State_stack& stack);

    void handle_event(const sf::Event& e) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::CircleShape m_player;
};
