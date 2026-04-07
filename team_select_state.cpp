// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================


#include "team_select_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"
#include "button.hpp"
#include "stateid.hpp"

TeamSelectState::TeamSelectState(StateStack& stack, Context context)
    : State(stack, context)
    , m_title(context.fonts->Get(FontID::kMain))
	, m_mode_text(context.fonts->Get(FontID::kMain))
    , m_name_text(context.fonts->Get(FontID::kMain))
    , m_fire_text(context.fonts->Get(FontID::kMain))
    , m_water_text(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f view_size = context.window->getView().getSize();

    m_title.setString("Team Select");
    m_title.setCharacterSize(48);
    Utility::CentreOrigin(m_title);
    m_title.setPosition({ view_size.x * 0.5f, view_size.y * 0.12f });

    m_mode_text.setCharacterSize(22);
    Utility::CentreOrigin(m_mode_text);
    m_mode_text.setPosition({ view_size.x * 0.5f, view_size.y * 0.18f });

    m_name_text.setCharacterSize(28);
    m_name_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.25f });

    m_fire_text.setCharacterSize(28);
    m_fire_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.36f });

    m_water_text.setCharacterSize(28);
    m_water_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.44f });

    m_hint.setString("Type nickname");
    m_hint.setCharacterSize(18);
    Utility::CentreOrigin(m_hint);
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.62f });

    auto join_fire = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    join_fire->SetText("Join Fire");
    join_fire->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.72f });
    join_fire->SetCallback([this]()
        {
            if (m_fire_count >= m_team_limit) return;
            if (m_fire_count > m_water_count) return;

            auto& settings = *GetContext().settings;

            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Fire;

            GetContext().sounds->Play(SoundID::kButton);

            RequestStackClear();
            RequestStackPush(StateID::kGame);
        });

    auto join_water = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    join_water->SetText("Join Water");
    join_water->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.80f });
    join_water->SetCallback([this]()
        {
            if (m_water_count >= m_team_limit) return;
            if (m_water_count > m_fire_count) return;

            auto& settings = *GetContext().settings;

            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Water;

            GetContext().sounds->Play(SoundID::kButton);

            RequestStackClear();
            RequestStackPush(StateID::kGame);
        });

    auto spectate = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    spectate->SetText("Spectate");
    spectate->setPosition({ view_size.x * 0.72f, view_size.y * 0.72f });
    spectate->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;

            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Spectator;

            GetContext().sounds->Play(SoundID::kButton);

            RequestStackClear();
            RequestStackPush(StateID::kGame);
        });

    auto back = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    back->SetText("Back");
    back->setPosition({ view_size.x * 0.72f, view_size.y * 0.80f });
    back->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackPop();
        });

    m_gui.Pack(join_fire);
    m_gui.Pack(join_water);
    m_gui.Pack(spectate);
    m_gui.Pack(back);

    refresh_text();
}

    void TeamSelectState::refresh_text()
    {
        const auto& settings = *GetContext().settings;

        std::string mode = "Offline";
        if (settings.network_role == GameSettings::NetworkRole::Host)
            mode = "Mode: Host";
        else if (settings.network_role == GameSettings::NetworkRole::Client)
            mode = "Mode: Client";

        m_mode_text.setString(mode);
        Utility::CentreOrigin(m_mode_text);

        m_fire_count = settings.latest_fire_count;
        m_water_count = settings.latest_water_count;
        m_team_limit = settings.team_limit;

        m_name_text.setString("Nickname: " + m_nickname);
        m_fire_text.setString("Fire Team: " + std::to_string(m_fire_count) + "/" + std::to_string(m_team_limit));
        m_water_text.setString("Water Team: " + std::to_string(m_water_count) + "/" + std::to_string(m_team_limit));
    }

void TeamSelectState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    sf::RectangleShape overlay;
    overlay.setSize(GetContext().window->getView().getSize());
    overlay.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(overlay);

    target.draw(m_title);
	target.draw(m_mode_text);
    target.draw(m_name_text);
    target.draw(m_fire_text);
    target.draw(m_water_text);
    target.draw(m_hint);
    target.draw(m_gui);
}

bool TeamSelectState::Update(sf::Time)
{
    return true;
}

bool TeamSelectState::HandleEvent(const sf::Event& event)
{
    m_gui.HandleEvent(event);

    if (const auto* text = event.getIf<sf::Event::TextEntered>())
    {
        unsigned int unicode = text->unicode;

        if (unicode >= 32 && unicode <= 126 && m_nickname.size() < 14)
        {
            m_nickname += static_cast<char>(unicode);
            refresh_text();
        }
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->scancode == sf::Keyboard::Scancode::Backspace && !m_nickname.empty())
        {
            m_nickname.pop_back();
            refresh_text();
        }
        else if (key->scancode == sf::Keyboard::Scancode::Escape)
        {
            RequestStackPop();
        }
    }

    return false;
}
