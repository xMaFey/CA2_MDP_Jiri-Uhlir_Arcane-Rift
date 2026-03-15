// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "settings_state.hpp"
#include "utility.hpp"
#include "fontID.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_title(context.fonts->Get(FontID::kMain))
	, m_controls_text(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f view_size = context.window->getView().getSize();

    m_title.setString("Settings");
    m_title.setCharacterSize(48);
    Utility::CentreOrigin(m_title);
    m_title.setPosition({ view_size.x * 0.5f, view_size.y * 0.35f });

    m_controls_text.setCharacterSize(28);
    Utility::CentreOrigin(m_controls_text);
    m_controls_text.setPosition({ view_size.x * 0.5f, view_size.y * 0.42f });

    m_hint.setString("Press ESC or Backspace to return");
    m_hint.setCharacterSize(20);
    Utility::CentreOrigin(m_hint);
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.72f });

	//toggle controls button
    auto toggle_controls = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    toggle_controls->SetText("Toggle Controls");
    toggle_controls->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.55f });
    toggle_controls->SetCallback([this]()
        {
            auto& s = *GetContext().settings;
            if (s.controls == GameSettings::ControlScheme::WASD)
                s.controls = GameSettings::ControlScheme::Arrows;
            else
                s.controls = GameSettings::ControlScheme::WASD;

            GetContext().sounds->Play(SoundID::kButton);
            refresh_text();
        });

    // back button
    auto back = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    back->SetText("Back");
    back->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });
    back->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.82f });

	m_gui.Pack(toggle_controls);
    m_gui.Pack(back);

	refresh_text();
}

void SettingsState::refresh_text()
{
    const auto& s = *GetContext().settings;
    m_controls_text.setString(
        std::string("Controls: ") +
        (s.controls == GameSettings::ControlScheme::WASD ? "WASD + J/K" : "Arrows + 1/2")
    );
    Utility::CentreOrigin(m_controls_text);
}

void SettingsState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    sf::RectangleShape overlay;
    overlay.setSize(GetContext().window->getView().getSize());
    overlay.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(overlay);

    target.draw(m_title);
	target.draw(m_controls_text);
    target.draw(m_hint);
	target.draw(m_gui);
}

bool SettingsState::Update(sf::Time)
{
    return false;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
    // let GUI handle mouse
    m_gui.HandleEvent(event);

    // keyboard shortcuts to go back
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->scancode == sf::Keyboard::Scancode::Escape ||
            key->scancode == sf::Keyboard::Scancode::Backspace)
        {
            RequestStackPop(); // go back to MenuState underneath
        }
    }

    return false;
}
