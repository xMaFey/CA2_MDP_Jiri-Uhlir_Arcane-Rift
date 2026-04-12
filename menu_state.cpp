// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "menu_state.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "button.hpp"
#include "stateid.hpp"

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    m_host_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);

    m_host_button->setPosition(sf::Vector2f(100, 250));
    m_host_button->SetText("Host Game");
    m_host_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.network_role = GameSettings::NetworkRole::Host;
            settings.server_port = 53000;

            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kTeamSelect);
        });

    m_join_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);

    m_join_button->setPosition(sf::Vector2f(100, 300));
    m_join_button->SetText("Join Game");
    m_join_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.network_role = GameSettings::NetworkRole::Client;
            settings.server_port = 53000;

            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kTeamSelect);
        });

    m_settings_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);

    m_settings_button->setPosition(sf::Vector2f(100, 350));
    m_settings_button->SetText("Settings");
    m_settings_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kSettings);
        });

    m_exit_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);

    m_exit_button->setPosition(sf::Vector2f(100, 400));
    m_exit_button->SetText("Exit");
    m_exit_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });

    m_gui_container.Pack(m_host_button);
    m_gui_container.Pack(m_join_button);
    m_gui_container.Pack(m_settings_button);
    m_gui_container.Pack(m_exit_button);

    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();

    const sf::Vector2f viewSize(
        static_cast<float>(context.window->getSize().x),
        static_cast<float>(context.window->getSize().y)
    );

    if (texSize.x > 0 && texSize.y > 0)
    {
        const float sx = viewSize.x / static_cast<float>(texSize.x);
        const float sy = viewSize.y / static_cast<float>(texSize.y);
        m_background_sprite.setScale({ sx, sy });
    }

    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);

    rebuild_layout(context.window->getSize());
}

void MenuState::rebuild_layout(sf::Vector2u new_size)
{
    const sf::Vector2f viewSize(static_cast<float>(new_size.x), static_cast<float>(new_size.y));

    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();
    if (texSize.x > 0 && texSize.y > 0)
    {
        const float sx = viewSize.x / static_cast<float>(texSize.x);
        const float sy = viewSize.y / static_cast<float>(texSize.y);
        m_background_sprite.setScale({ sx, sy });
    }

    // Keep menu buttons on the left side, but scale their placement nicely
    // with the window size instead of hardcoded pixels.
    const float leftX = viewSize.x * 0.10f;
    const float startY = viewSize.y * 0.32f;
    const float gap = viewSize.y * 0.065f;

    if (m_host_button)
        m_host_button->setPosition({ leftX, startY });

    if (m_join_button)
        m_join_button->setPosition({ leftX, startY + gap });

    if (m_settings_button)
        m_settings_button->setPosition({ leftX, startY + gap * 2.f });

    if (m_exit_button)
        m_exit_button->setPosition({ leftX, startY + gap * 3.f });
}

void MenuState::OnResize(sf::Vector2u new_size)
{
    rebuild_layout(new_size);
}

void MenuState::Draw(sf::RenderTarget& target)
{
    target.setView(target.getDefaultView());
    target.draw(m_background_sprite);
    target.draw(m_gui_container);
}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

bool MenuState::HandleEvent(const sf::Event& event)
{
    m_gui_container.HandleEvent(event);
    return true;
}