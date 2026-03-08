// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "play_state.hpp"
#include "pause_state.hpp"
#include "state_stack.hpp"

Play_state::Play_state(State_stack& stack)
    : State(stack)
{
    m_player.setRadius(25.f);
    m_player.setPosition({ 200.f, 200.f });
}

void Play_state::handle_event(const sf::Event& e)
{
    // Esc -> push Pause_state on top of Play_state
    if (e.is<sf::Event::KeyPressed>())
    {
        const auto& kp = *e.getIf<sf::Event::KeyPressed>();
        if (kp.code == sf::Keyboard::Key::Escape)
        {
            m_stack.push<Pause_state>();
        }
    }
}

void Play_state::update(sf::Time dt)
{
    // Simple movement (WASD)
    const float speed = 260.f;
    sf::Vector2f dir{ 0.f, 0.f };

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) dir.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) dir.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) dir.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) dir.x += 1.f;

    m_player.move(dir * speed * dt.asSeconds());
}

void Play_state::render(sf::RenderWindow& window)
{
    window.draw(m_player);
}
