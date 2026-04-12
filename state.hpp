// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include "stateid.hpp"
#include "sound_player.hpp"
#include "music_player.hpp"
#include "game_settings.hpp"
#include "network_manager.hpp"

class StateStack;


class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, SoundPlayer& sounds, MusicPlayer& music, GameSettings& settings, NetworkManager& network);
		
		//TODO unique_ptr rather than raw pointers here?
		sf::RenderTarget* target;
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;

		SoundPlayer* sounds;
		MusicPlayer* music;

		GameSettings* settings;
		NetworkManager* network;
	};

public:
	State(StateStack& stack, Context context);
	virtual ~State();
	virtual void Draw(sf::RenderTarget& target) = 0;
	virtual bool Update(sf::Time dt) = 0;
	virtual bool HandleEvent(const sf::Event& event) = 0;
	virtual void OnResize(sf::Vector2u new_size) {}

protected:
	void RequestStackPush(StateID state_id);
	void RequestStackPop();
	void RequestStackClear();

	Context GetContext() const;

private:
	StateStack* m_stack;
	Context m_context;
};

