// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "game_over_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"
#include "stateid.hpp"
#include "button.hpp"

GameOverState::GameOverState(StateStack& stack, Context context)
    : State(stack, context)
    , m_background_sprite(
        context.textures->Get(
            context.settings->last_winner_team == GameSettings::Team::Water
            ? TextureID::kBluePlayerWin
            : TextureID::kOrangePlayerWin
        )
    )
    , m_overlay()
    , m_title(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
    , m_gui()
{
    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);

    sf::Vector2f view_size(
        static_cast<float>(context.window->getSize().x),
        static_cast<float>(context.window->getSize().y)
    );


	// scale background to fit view
    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();
	if (texSize.x > 0 && texSize.y > 0)
    {
			const float sx = view_size.x / static_cast<float>(texSize.x);
			const float sy = view_size.y / static_cast<float>(texSize.y);
            m_background_sprite.setScale({ sx, sy });
    }

    // dark overlay
	m_overlay.setSize(view_size);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 110));

    // winner text
	m_title.setCharacterSize(64);
    m_title.setFillColor(sf::Color::White);
    m_title.setOutlineThickness(3.f);
    m_title.setOutlineColor(sf::Color::Black);

    if (context.settings->last_winner_team == GameSettings::Team::Water)
        m_title.setString("Water Team Wins!");
    else if (context.settings->last_winner_team == GameSettings::Team::Fire)
        m_title.setString("Fire Team Wins!");
    else
        m_title.setString("Match Over");

    Utility::CentreOrigin(m_title);
    m_title.setPosition({ view_size.x / 2.f, view_size.y / 3.f });

	//m_hint.setString("Press Enter to play again, or Escape for menu");
	m_hint.setCharacterSize(24);
    m_hint.setFillColor(sf::Color::White);
    m_hint.setOutlineThickness(2.f);
	m_hint.setOutlineColor(sf::Color::Black);

	Utility::CentreOrigin(m_hint);
	m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.38f });

    // buttons
    m_play_again_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_play_again_button->SetText("Play Again (Enter)");
    m_play_again_button->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.52f });
    m_play_again_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);

            // Go back to team select before starting a new match.
            RequestStackClear();
            RequestStackPush(StateID::kTeamSelect);
        });

    m_back_to_menu_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_back_to_menu_button->SetText("Back to Menu (Escape)");
    m_back_to_menu_button->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.62f });
    m_back_to_menu_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackClear();
            RequestStackPush(StateID::kMenu);
		});

    m_gui.Pack(m_play_again_button);
    m_gui.Pack(m_back_to_menu_button);

    rebuild_layout(context.window->getSize());
}

void GameOverState::Draw(sf::RenderTarget& target)
{
    target.setView(target.getDefaultView());

    target.draw(m_background_sprite);
    target.draw(m_overlay);
    target.draw(m_title);
    target.draw(m_hint);
	target.draw(m_gui);
}

bool GameOverState::Update(sf::Time)
{
    return true;
}

bool GameOverState::HandleEvent(const sf::Event& event)
{
    m_gui.HandleEvent(event);

    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (!key) return false;

    if (key->scancode == sf::Keyboard::Scancode::Enter)
    {
        // Restart flow through team select.
        RequestStackClear();
        RequestStackPush(StateID::kTeamSelect);
        return true;
    }

    if (key->scancode == sf::Keyboard::Scancode::Escape)
    {
        // Back to menu
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
        return true;
    }

    return false;
}

void GameOverState::rebuild_layout(sf::Vector2u new_size)
{
    const sf::Vector2f view_size(static_cast<float>(new_size.x), static_cast<float>(new_size.y));

    const sf::Vector2u texSize = m_background_sprite.getTexture().getSize();
    if (texSize.x > 0 && texSize.y > 0)
    {
        const float sx = view_size.x / static_cast<float>(texSize.x);
        const float sy = view_size.y / static_cast<float>(texSize.y);
        m_background_sprite.setScale({ sx, sy });
    }

    m_overlay.setSize(view_size);
    m_title.setPosition({ view_size.x / 2.f, view_size.y / 3.f });
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.38f });

    if (m_play_again_button)
        m_play_again_button->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.52f });

    if (m_back_to_menu_button)
        m_back_to_menu_button->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.62f });
}

void GameOverState::OnResize(sf::Vector2u new_size)
{
    rebuild_layout(new_size);
}
