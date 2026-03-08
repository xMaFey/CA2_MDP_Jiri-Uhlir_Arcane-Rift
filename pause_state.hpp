// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "State.hpp"
#include "container.hpp"
#include "button.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

class PauseState : public State
{
public:
	PauseState(StateStack& stack, Context context);
	virtual void Draw(sf::RenderTarget& target) override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	sf::Text m_paused_text;
	sf::Text m_instruction_text;
	gui::Container m_gui;
};

