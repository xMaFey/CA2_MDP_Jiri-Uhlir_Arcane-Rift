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
    auto host_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);

    host_button->setPosition(sf::Vector2f(100, 250));
    host_button->SetText("Host Game");
    host_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.network_role = GameSettings::NetworkRole::Host;
            settings.server_port = 53000;

            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kTeamSelect);
        });

    auto join_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    join_button->setPosition(sf::Vector2f(100, 300));
    join_button->SetText("Join Game");
    join_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.network_role = GameSettings::NetworkRole::Client;
            settings.server_port = 53000;

            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kTeamSelect);
        });

    auto settings_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    settings_button->setPosition(sf::Vector2f(100, 350));
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kSettings);
        });

    auto exit_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    exit_button->setPosition(sf::Vector2f(100, 400));
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });

    m_gui_container.Pack(host_button);
    m_gui_container.Pack(join_button);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

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