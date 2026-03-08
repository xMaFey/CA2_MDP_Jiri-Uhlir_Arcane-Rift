// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "title_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"

TitleState::TitleState(StateStack& stack, Context context) : State(stack, context), m_show_text(true), m_text_effect_time(sf::Time::Zero), m_background_sprite(context.textures->Get(TextureID::kTitleScreen)), m_text(context.fonts->Get(FontID::kMain))
{
    m_text.setString("Press any key to continue");
    Utility::CentreOrigin(m_text);
    m_text.setPosition(context.window->getView().getSize() / 2.f);

    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);
}

void TitleState::Draw(sf::RenderTarget& target)
{
    target.setView(GetContext().window->getDefaultView());
    target.draw(m_background_sprite);

    if (m_show_text)
    {
        target.draw(m_text);
    }
}

bool TitleState::Update(sf::Time dt)
{
    m_text_effect_time += dt;
    if (m_text_effect_time >= sf::seconds(0.5))
    {
        m_show_text = !m_show_text;
        m_text_effect_time = sf::Time::Zero;
    }
    return true;
}

bool TitleState::HandleEvent(const sf::Event& event)
{
    // Any key
    if (event.is<sf::Event::KeyPressed>())
    {
        RequestStackPop();
        RequestStackPush(StateID::kMenu);
        return true;
    }

    // Any mouse click
    if (event.is<sf::Event::MouseButtonPressed>())
    {
        RequestStackPop();
        RequestStackPush(StateID::kMenu);
        return true;
    }

    return true;
}

