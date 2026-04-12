// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "state.hpp"
#include "container.hpp"
#include "button.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

class GameOverState : public State
{
public:
    GameOverState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;
    void OnResize(sf::Vector2u new_size) override;

private:
    void rebuild_layout(sf::Vector2u new_size);

private:
    sf::RectangleShape m_overlay;
    sf::Text m_title;
    sf::Text m_hint;
    sf::Sprite m_background_sprite;

    gui::Container m_gui;
    gui::Button::Ptr m_play_again_button;
    gui::Button::Ptr m_back_to_menu_button;
};
