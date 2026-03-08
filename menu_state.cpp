// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "menu_state.hpp"
#include "fontID.hpp"
#include <SFML/Graphics/Text.hpp>
#include "utility.hpp"
#include "menu_options.hpp"
#include "button.hpp"

MenuState::MenuState(StateStack& stack, Context context) : State(stack, context), m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    auto play_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    play_button->setPosition(sf::Vector2f(100, 250));
    play_button->SetText("Play");
    play_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
            RequestStackPush(StateID::kGame);
        });

    auto settings_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    settings_button->setPosition(sf::Vector2f(100, 300));
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPush(StateID::kSettings);
        });

    auto exit_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    exit_button->setPosition(sf::Vector2f(100, 350));
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });

    m_gui_container.Pack(play_button);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);
}

void MenuState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    target.setView(window.getDefaultView());
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