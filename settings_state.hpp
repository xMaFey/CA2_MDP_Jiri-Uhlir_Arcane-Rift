// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "state.hpp"
#include "container.hpp"
#include "button.hpp"
#include <SFML/Graphics/Text.hpp>

class SettingsState : public State
{
public:
    SettingsState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

    void OnResize(sf::Vector2u new_size) override;
    void rebuild_layout(sf::Vector2u new_size);

private:
    void refresh_text();

private:
    sf::Text m_title;
    sf::Text m_controls_text;
    sf::Text m_hint;

	gui::Container m_gui;

    gui::Button::Ptr m_toggle_controls_button;
    gui::Button::Ptr m_back_button;
};
