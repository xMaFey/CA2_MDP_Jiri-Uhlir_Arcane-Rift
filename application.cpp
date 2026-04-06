// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "application.hpp"
#include "constants.hpp"
#include "fontID.hpp"
#include "game_state.hpp"
#include "title_state.hpp"
#include "menu_state.hpp"
#include "settings_state.hpp"
#include "game_over_state.hpp"
#include "team_select_state.hpp"
#include <fstream>
#include <iostream>

Application::Application() : m_window(sf::VideoMode({ 1280, 720 }), "States", sf::Style::Close)
, m_textures()
, m_fonts()
, m_stack(State::Context(m_window, m_textures, m_fonts, m_sounds, m_music, m_settings, m_network))
, m_sounds()
, m_music()
, m_scene_texture(m_window.getSize())
, m_scene_sprite(m_scene_texture.getTexture())
{
	m_window.setKeyRepeatEnabled(false);

	// load server IP from file
		std::ifstream file("ip.txt");
		if (file.is_open())
		{
			std::getline(file, m_settings.server_ip);

			if (!m_settings.server_ip.empty())
			{
				std::cout << "Loaded server IP: " << m_settings.server_ip << "\n";
			}
			else
			{
				m_settings.server_ip = "127.0.0.1";
				std::cout << "ip.txt was empty, using default IP\n";
			}
		}
		else
		{
			m_settings.server_ip = "127.0.0.1";
			std::cout << "Could not open ip.txt, using default IP\n";
		}

		std::cout << "Connecting to: " << m_settings.server_ip << ":" << m_settings.server_port << "\n";

	m_fonts.Load(FontID::kMain, "Media/Fonts/Sansation.ttf");
	m_textures.Load(TextureID::kEagle, "Media/Textures/Eagle.png");
	m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
	m_textures.Load(TextureID::kButtonNormal, "Media/Textures/ButtonNormal.png");
	m_textures.Load(TextureID::kButtonSelected, "Media/Textures/ButtonSelected.png");
	m_textures.Load(TextureID::kButtonActivated, "Media/Textures/ButtonPressed.png");
	//m_textures.Load(TextureID::kBluePlayerWin, "Media/Textures/BluePlayerWin.png");
	//m_textures.Load(TextureID::kOrangePlayerWin, "Media/Textures/OrangePlayerWin.png");

	m_sounds.Load(SoundID::kButton, "Media/Audio/sfx/button.wav");
	m_sounds.Load(SoundID::kDash, "Media/Audio/sfx/dash.wav");
	m_sounds.Load(SoundID::kFireSpell, "Media/Audio/sfx/fire_spell.wav");
	m_sounds.Load(SoundID::kWaterSpell, "Media/Audio/sfx/water_spell.wav");
	m_sounds.Load(SoundID::kFireHit, "Media/Audio/sfx/fire_hit.wav");
	m_sounds.Load(SoundID::kWaterHit, "Media/Audio/sfx/water_hit.wav");

	// load vignette shader
	m_vignette_ok = m_vignette_shader.loadFromFile("Media/Shaders/vignette.frag", sf::Shader::Type::Fragment);

	if (m_vignette_ok)
	{
		// set uniforms before drawing with shader
		m_vignette_shader.setUniform("resolution", sf::Glsl::Vec2((float)m_window.getSize().x, (float)m_window.getSize().y));

		m_vignette_shader.setUniform("strength", 0.65f);
	}

	// volume
	m_sounds.SetVolume(75.f);

	RegisterStates();
	m_stack.PushState(StateID::kTitle);
}

void Application::Run()
{
	sf::Clock clock;
	sf::Time time_since_last_update = sf::Time::Zero;
	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInput();
			Update(sf::seconds(kTimePerFrame));

			if (m_stack.IsEmpty())
			{
				m_window.close();
			}
		}
		Render();
	}
}

void Application::ProcessInput()
{
	while (const std::optional event = m_window.pollEvent())
	{
		m_stack.HandleEvent(*event);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}

	}
}

void Application::Update(sf::Time dt)
{
	m_stack.Update(dt);
}

void Application::Render()
{
	// draw state stack into offscreen texture
	m_scene_texture.clear();
	m_scene_texture.setView(m_window.getView());
	m_stack.Draw(m_scene_texture);
	m_scene_texture.display();

	// draw offxreen texture to the real window, with shader
	m_window.clear();

	if (m_vignette_ok)
	{
		m_window.draw(m_scene_sprite, &m_vignette_shader);
	}
	else 
	{
		m_window.draw(m_scene_sprite);
	}

	m_window.display();
	/*m_window.clear();
	m_stack.Draw();
	m_window.display();*/
}

void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	m_stack.RegisterState<GameState>(StateID::kGame);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
	m_stack.RegisterState<GameOverState>(StateID::kGameOver);
	m_stack.RegisterState<TeamSelectState>(StateID::kTeamSelect);
}


