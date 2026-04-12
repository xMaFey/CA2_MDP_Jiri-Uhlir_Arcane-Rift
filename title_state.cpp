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

    const sf::Vector2f viewSize(
        static_cast<float>(context.window->getSize().x),
        static_cast<float>(context.window->getSize().y)
    );

    m_text.setPosition(viewSize / 2.f);

    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();

    if (texSize.x > 0 && texSize.y > 0)
    {
        const float sx = viewSize.x / static_cast<float>(texSize.x);
        const float sy = viewSize.y / static_cast<float>(texSize.y);
        m_background_sprite.setScale({ sx, sy });
    }

    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);

    rebuild_layout(context.window->getSize());
}

void TitleState::Draw(sf::RenderTarget& target)
{
    target.setView(target.getDefaultView());
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

void TitleState::OnResize(sf::Vector2u new_size)
{
    rebuild_layout(new_size);
}

void TitleState::rebuild_layout(sf::Vector2u new_size)
{
    const sf::Vector2f viewSize(static_cast<float>(new_size.x), static_cast<float>(new_size.y));

    m_text.setPosition(viewSize / 2.f);

    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();
    if (texSize.x > 0 && texSize.y > 0)
    {
        const float sx = viewSize.x / static_cast<float>(texSize.x);
        const float sy = viewSize.y / static_cast<float>(texSize.y);
        m_background_sprite.setScale({ sx, sy });
    }
}

