// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"
#include "sound_player.hpp"
#include "music_player.hpp"


class Application
{
public:
	Application();
	void Run();

private:
	void ProcessInput();
	void Update(sf::Time dt);
	void Render();
	void RegisterStates();

private:
	sf::RenderWindow m_window;
	Player m_player;

	TextureHolder m_textures;
	FontHolder m_fonts;

	StateStack m_stack;

	SoundPlayer m_sounds;
	MusicPlayer m_music;

	sf::RenderTexture m_scene_texture;
	sf::Sprite m_scene_sprite;
	sf::Shader m_vignette_shader;
	bool m_vignette_ok = false;
};

