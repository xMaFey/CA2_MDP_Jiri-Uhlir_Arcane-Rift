// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "state.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include "container.hpp"

class MenuState : public State
{
public:
	MenuState(StateStack& stack, Context context);
	virtual void Draw(sf::RenderTarget& target) override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;
	void UpdateOptionText();

private:
	sf::Sprite m_background_sprite;
	gui::Container m_gui_container;
};

