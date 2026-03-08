// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "settings_state.hpp"
#include "utility.hpp"
#include "fontID.hpp"
#include "stateid.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_title(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f view_size = context.window->getView().getSize();

    m_title.setString("Settings (placeholder)");
    m_title.setCharacterSize(48);
    Utility::CentreOrigin(m_title);
    m_title.setPosition({ view_size.x * 0.5f, view_size.y * 0.35f });

    m_hint.setString("Press ESC or Backspace to return");
    m_hint.setCharacterSize(20);
    Utility::CentreOrigin(m_hint);
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.55f });

    // back button
    auto back = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    back->SetText("Back");
    back->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });
    back->setPosition({ view_size.x * 0.5f, view_size.y * 0.62f });

    m_gui.Pack(back);
}

void SettingsState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    target.draw(m_title);
    target.draw(m_hint);
	target.draw(m_gui);
}

bool SettingsState::Update(sf::Time)
{
    return true; // keep updating states below? doesn't matter here
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
	// let GUI handle mouse
	m_gui.HandleEvent(event);

	// keyboard shortcuts to go back
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (!key) return false;

    if (key->scancode == sf::Keyboard::Scancode::Escape ||
        key->scancode == sf::Keyboard::Scancode::Backspace)
    {
        RequestStackPop(); // go back to MenuState underneath
    }
    return false;
}
