// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "state.hpp"
#include "container.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

class GameOverState : public State
{
public:
	// winner: 1 = p1, 2 = p2
    GameOverState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    sf::RectangleShape m_overlay;
    sf::Text m_title;
    sf::Text m_hint;
    sf::Sprite m_background_sprite;

    gui::Container m_gui;
};
