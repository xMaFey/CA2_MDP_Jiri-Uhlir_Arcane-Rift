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
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
	, m_overlay()
    , m_title(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
    , m_gui()
{
    GetContext().music->PlayLoop("Media/Audio/music/background.wav", 30.f);

    sf::Vector2f view_size = context.window->getView().getSize();


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
    

	Utility::CentreOrigin(m_title);
	m_title.setPosition({ view_size.x / 2.f, view_size.y / 3.f });
    m_title.setString("Match Over");

	m_hint.setString("Press Enter to play again, or Escape for menu");
	m_hint.setCharacterSize(24);
    m_hint.setFillColor(sf::Color::White);
    m_hint.setOutlineThickness(2.f);
	m_hint.setOutlineColor(sf::Color::Black);
	Utility::CentreOrigin(m_hint);
	m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.38f });

    // buttons
	auto play_again = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    play_again->SetText("Play Again (Enter)");
	play_again->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.52f });
    play_again->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackClear();
            RequestStackPush(StateID::kGame);
		});

    auto back_to_menu = std::make_shared<gui::Button>(*context.fonts, *context.textures);
	back_to_menu->SetText("Back to Menu (Escape)");
	back_to_menu->setPosition({ view_size.x / 2.f - 100.f, view_size.y * 0.62f });
    back_to_menu->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);
            RequestStackClear();
            RequestStackPush(StateID::kMenu);
		});

	m_gui.Pack(play_again);
	m_gui.Pack(back_to_menu);
}

void GameOverState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

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
        // Restart the match: clear stack and go straight to game
        RequestStackClear();
        RequestStackPush(StateID::kGame);
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
