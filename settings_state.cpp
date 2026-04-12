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
    sf::Vector2f view_size(
        static_cast<float>(context.window->getSize().x),
        static_cast<float>(context.window->getSize().y)
    );

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
    m_toggle_controls_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_toggle_controls_button->SetText("Toggle Controls");
    m_toggle_controls_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.55f });
    m_toggle_controls_button->SetCallback([this]()
        {
            auto& s = *GetContext().settings;

            const bool is_wasd =
                s.local_keys.up == sf::Keyboard::Scancode::W &&
                s.local_keys.down == sf::Keyboard::Scancode::S &&
                s.local_keys.left == sf::Keyboard::Scancode::A &&
                s.local_keys.right == sf::Keyboard::Scancode::D &&
                s.local_keys.shoot == sf::Keyboard::Scancode::J &&
                s.local_keys.dash == sf::Keyboard::Scancode::K;

            if (is_wasd)
            {
                s.local_keys.up = sf::Keyboard::Scancode::Up;
                s.local_keys.down = sf::Keyboard::Scancode::Down;
                s.local_keys.left = sf::Keyboard::Scancode::Left;
                s.local_keys.right = sf::Keyboard::Scancode::Right;
                s.local_keys.shoot = sf::Keyboard::Scancode::Num1;
                s.local_keys.dash = sf::Keyboard::Scancode::Num2;
            }
            else
            {
                s.local_keys.up = sf::Keyboard::Scancode::W;
                s.local_keys.down = sf::Keyboard::Scancode::S;
                s.local_keys.left = sf::Keyboard::Scancode::A;
                s.local_keys.right = sf::Keyboard::Scancode::D;
                s.local_keys.shoot = sf::Keyboard::Scancode::J;
                s.local_keys.dash = sf::Keyboard::Scancode::K;
            }

            GetContext().sounds->Play(SoundID::kButton);
            refresh_text();
        });

    // back button
    m_back_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_back_button->SetText("Back");
    m_back_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });
    m_back_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.82f });

    m_gui.Pack(m_toggle_controls_button);
    m_gui.Pack(m_back_button);

	refresh_text();

    rebuild_layout(context.window->getSize());
}

void SettingsState::refresh_text()
{
    const auto& s = *GetContext().settings;

    const bool is_wasd =
        s.local_keys.up == sf::Keyboard::Scancode::W &&
        s.local_keys.down == sf::Keyboard::Scancode::S &&
        s.local_keys.left == sf::Keyboard::Scancode::A &&
        s.local_keys.right == sf::Keyboard::Scancode::D &&
        s.local_keys.shoot == sf::Keyboard::Scancode::J &&
        s.local_keys.dash == sf::Keyboard::Scancode::K;

    m_controls_text.setString(
        std::string("Controls: ") +
        (is_wasd ? "WASD + J/K" : "Arrows + 1/2")
    );

    Utility::CentreOrigin(m_controls_text);
}

void SettingsState::Draw(sf::RenderTarget& target)
{
    target.setView(target.getDefaultView());

    sf::RectangleShape overlay;
    overlay.setSize(target.getView().getSize());
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

void SettingsState::rebuild_layout(sf::Vector2u new_size)
{
    const sf::Vector2f view_size(static_cast<float>(new_size.x), static_cast<float>(new_size.y));

    m_title.setPosition({ view_size.x * 0.5f, view_size.y * 0.35f });
    m_controls_text.setPosition({ view_size.x * 0.5f, view_size.y * 0.42f });
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.72f });

    if (m_toggle_controls_button)
        m_toggle_controls_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.55f });

    if (m_back_button)
        m_back_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.82f });
}

void SettingsState::OnResize(sf::Vector2u new_size)
{
    rebuild_layout(new_size);
}