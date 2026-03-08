// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
// Game owns the window and controls the main loop
// It also registers all states and chooses which state to start with

#include <SFML/Graphics.hpp>
#include "state_stack.hpp"

class Game
{
public:
    Game();     // sets up window + registers states + pushes Title_state
    void run(); // main loop

private:
    void process_events();

    // Update the current state based on dt
    void update(sf::Time dt);

    // Draw the current state(s)
    void render();

    sf::RenderWindow m_window;
    State_stack m_states;
    sf::Clock m_clock;
};
