// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "state.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

class TitleState : public State
{
public: 
	TitleState(StateStack& stack, Context context);
	virtual void Draw(sf::RenderTarget& target) override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

	void OnResize(sf::Vector2u new_size) override;
	void rebuild_layout(sf::Vector2u new_size);

private:
	sf::Sprite m_background_sprite;
	sf::Text m_text;

	bool m_show_text;
	sf::Time m_text_effect_time;
};

