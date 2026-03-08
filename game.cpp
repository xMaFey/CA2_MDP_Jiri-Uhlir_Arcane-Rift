// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

// Implements the Game loop and sets up state flow.

#include "game.hpp"

// Include states so register_state<T>() knows the full class definition
#include "title_state.hpp"
#include "menu_state.hpp"
#include "play_state.hpp"
#include "pause_state.hpp"

Game::Game()
    : m_window(sf::VideoMode({ 960, 540 }), "CA1 - GameDash")
    , m_states(m_window)
{
    // Limit FPS so movement feels consistent
    m_window.setFramerateLimit(60);

    // Register all states with the stack
    m_states.register_state<Title_state>();
    m_states.register_state<Menu_state>();
    m_states.register_state<Play_state>();
    m_states.register_state<Pause_state>();

    // Start at title screen
    m_states.push<Title_state>();
}

void Game::run()
{
    // Main game loop
    while (m_window.isOpen())
    {
        process_events();

        // dt = time since last frame
        const sf::Time dt = m_clock.restart();

        update(dt);
        render();
    }
}

void Game::process_events()
{
    // Poll all window/system events
    while (const auto e = m_window.pollEvent())
    {
        // If user clicks X, close the window
        if (e->is<sf::Event::Closed>())
            m_window.close();

        // Send the event to the active state
        m_states.handle_event(*e);
    }
}

void Game::update(sf::Time dt)
{
    // Update the active state
    m_states.update(dt);

    // If stack becomes empty, exit the game
    if (m_states.empty())
        m_window.close();
}

void Game::render()
{
    m_window.clear();
    m_states.render();
    m_window.display();
}
