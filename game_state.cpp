// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "game_state.hpp"
#include "fontID.hpp"
#include "stateid.hpp"
#include <algorithm>
#include <sstream>
#include <random>
#include <iostream>
#include <cmath>
#include <SFML/Network/IpAddress.hpp>
#include "game_settings.hpp"
#include "sound_id.hpp"
#include "utility.hpp"
#include "profile_data.hpp"

namespace
{
    constexpr int kPendingLocalPlayerId = -100;
    constexpr sf::Time kTeamSwitchDelay = sf::seconds(3.f);

    // checking if the player is actually in one of the two fighting teams
    bool is_combat_team(GameSettings::Team t)
    {
        return t == GameSettings::Team::Fire || t == GameSettings::Team::Water;
    }

    // applying team visuals to the player slot
    void apply_team_visuals(PlayerEntity& player, GameSettings::Team team)
    {
        if (team == GameSettings::Team::Fire)
        {
            player.set_color(sf::Color(255, 140, 90));
            player.set_animation_root("Media/Assets/Characters/wizard_orange/animations/");
        }
        else if (team == GameSettings::Team::Water)
        {
            player.set_color(sf::Color(70, 200, 255));
            player.set_animation_root("Media/Assets/Characters/wizard_blue/animations/");
        }
    }

    void apply_spectator_visuals(PlayerEntity& player)
    {
        // Neutral visuals for spectator mode.
        player.set_color(sf::Color(180, 180, 180));
    }

    // converting network team id into local enum
    GameSettings::Team decode_team(int t)
    {
        if (t == static_cast<int>(NetTeam::Fire)) return GameSettings::Team::Fire;
        if (t == static_cast<int>(NetTeam::Water)) return GameSettings::Team::Water;
        return GameSettings::Team::Spectator;
    }

    // converting local enum into network team id
    int encode_team(GameSettings::Team t)
    {
        if (t == GameSettings::Team::Fire) return static_cast<int>(NetTeam::Fire);
        if (t == GameSettings::Team::Water) return static_cast<int>(NetTeam::Water);
        return static_cast<int>(NetTeam::Spectator);
    }

    // converting team enum to text for HUD
    std::string team_to_string(GameSettings::Team t)
    {
        if (t == GameSettings::Team::Fire) return "Fire";
        if (t == GameSettings::Team::Water) return "Water";
        return "Spectator";
    }

    // squared distance helper for respawn logic
    float dist2(sf::Vector2f a, sf::Vector2f b)
    {
        sf::Vector2f d = a - b;
        return d.x * d.x + d.y * d.y;
    }

    // default spawn positions for first few player ids
    sf::Vector2f spawn_for_player_id(int id)
    {
        static const std::vector<sf::Vector2f> spawns =
        {
            {300.f, 300.f},
            {2900.f, 300.f},
            {300.f, 1500.f},
            {2900.f, 1500.f},
            {1600.f, 300.f},
            {1600.f, 1500.f},
            {1600.f, 900.f},
            {700.f, 900.f},
            {2500.f, 900.f}
        };

        if (id >= 0 && id < static_cast<int>(spawns.size()))
            return spawns[id];

        return spawns.back();
    }
}

GameState::GameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_hud(context.fonts->Get(FontID::kMain))
    , m_pause_title(context.fonts->Get(FontID::kMain))
    , m_pause_options(context.fonts->Get(FontID::kMain))
{
    auto& settings = *GetContext().settings;

    // Start the world camera with the same size as the visible window.
    m_world_view = m_window.getDefaultView();

    GetContext().music->Stop();

    // Networking should already be running if we came here from the lobby.
    // Reuse the existing host/client connection instead of reconnecting.
    if (GetContext().network)
    {
        auto& network = *GetContext().network;

        if (settings.network_role == GameSettings::NetworkRole::Host)
        {
            m_host_session = std::make_unique<HostSession>(network);

            if (network.mode() != NetworkManager::Mode::Host || !network.is_connected())
            {
                const bool ok = m_host_session->start(settings.server_port);
                if (ok)
                    std::cout << "Host started on port " << settings.server_port << "\n";
                else
                    std::cout << "Host failed\n";
            }
        }
        else if (settings.network_role == GameSettings::NetworkRole::Client)
        {
            m_client_session = std::make_unique<ClientSession>(network);

            bool ready = false;

            if (network.mode() != NetworkManager::Mode::Client || !network.is_connected())
            {
                const auto ip = sf::IpAddress::resolve(settings.server_ip);

                bool ok = false;
                if (ip.has_value())
                    ok = m_client_session->connect(*ip, settings.server_port);

                std::cout << (ok ? "Client connected\n" : "Client failed to connect\n");
                ready = ok;
            }
            else
            {
                // Reusing the existing lobby connection.
                ready = true;
            }

            // Even when reusing the lobby connection, resend JoinInfo so the
            // host's GameState rebuilds the correct remote player slots.
            if (ready && m_client_session)
            {
                JoinInfoPacket joinInfo;
                joinInfo.player_id = -1;
                joinInfo.nickname = settings.nickname;
                joinInfo.team = encode_team(settings.chosen_team);

                m_client_session->send_join_info(joinInfo);
            }
        }
        else
        {
            network.disconnect();
        }
    }

    build_map();

    // building player slot list
    m_players.clear();

    PlayerSlot hostSlot;
    hostSlot.id = 0;
    hostSlot.nickname = (settings.network_role == GameSettings::NetworkRole::Client)
        ? "Host"
        : settings.nickname;
    hostSlot.team = GameSettings::Team::Spectator;
    hostSlot.connected = true;
    hostSlot.entity.set_position(spawn_for_player_id(0));

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        // Host always owns player id 0.
        m_local_player_id = 0;
        hostSlot.team = settings.chosen_team;

        m_players.push_back(std::move(hostSlot));
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        // Client does not know its real player id yet.
        // Use a special temporary negative id so it can never collide
        // with a real host-assigned network player id.
        m_local_player_id = -1;

        m_players.push_back(std::move(hostSlot));

        PlayerSlot pendingLocalSlot;
        pendingLocalSlot.id = kPendingLocalPlayerId;
        pendingLocalSlot.nickname = settings.nickname;
        pendingLocalSlot.team = settings.chosen_team;
        pendingLocalSlot.connected = true;
        pendingLocalSlot.entity.set_position(spawn_for_player_id(1));

        m_players.push_back(std::move(pendingLocalSlot));
    }
    else
    {
        // Offline fallback keeps two local slots.
        m_local_player_id = 0;
        hostSlot.team = GameSettings::Team::Fire;

        PlayerSlot offlineSecondSlot;
        offlineSecondSlot.id = 1;
        offlineSecondSlot.nickname = "Player";
        offlineSecondSlot.team = GameSettings::Team::Water;
        offlineSecondSlot.connected = true;
        offlineSecondSlot.entity.set_position(spawn_for_player_id(1));

        m_players.push_back(std::move(hostSlot));
        m_players.push_back(std::move(offlineSecondSlot));
    }

    // Apply visuals only after the player slots are already stored inside m_players.
    // This avoids creating sprites that reference textures before the PlayerEntity gets moved.
    for (auto& p : m_players)
    {
        if (is_combat_team(p.team))
            apply_team_visuals(p.entity, p.team);
        else
            apply_spectator_visuals(p.entity);
    }


    m_hud.setCharacterSize(20);
    m_hud.setPosition({ 14.f, 10.f });

    // pause menu visuals
    const sf::Vector2f viewSize(
        static_cast<float>(m_window.getSize().x),
        static_cast<float>(m_window.getSize().y)
    );

    m_pause_overlay.setSize(viewSize);
    m_pause_overlay.setFillColor(sf::Color(0, 0, 0, 170));

    m_pause_title.setString("Game Menu");
    m_pause_title.setCharacterSize(42);
    Utility::CentreOrigin(m_pause_title);
    m_pause_title.setPosition({ viewSize.x * 0.5f, viewSize.y * 0.18f });

    m_pause_options.setCharacterSize(26);
    Utility::CentreOrigin(m_pause_options);
    m_pause_options.setPosition({ viewSize.x * 0.5f, viewSize.y * 0.28f });

    build_pause_gui();
    update_pause_button_states();

    rebuild_pause_layout(m_window.getSize());

    // spawn points used when a player dies
    m_spawn_points =
    {
        // corners / near-corners
        {260.f, 220.f},
        {260.f, 1580.f},
        {2940.f, 220.f},
        {2940.f, 1580.f},

        // mid-left / mid-right
        {620.f, 900.f},
        {2580.f, 900.f},

        // upper-middle / lower-middle
        {1600.f, 260.f},
        {1600.f, 1540.f},

        // center-ish fallback
        {1600.f, 900.f}
    };
}

GameState::PlayerSlot* GameState::find_player(int id)
{
    for (auto& p : m_players)
    {
        if (p.id == id)
            return &p;
    }

    return nullptr;
}

const GameState::PlayerSlot* GameState::find_player(int id) const
{
    for (const auto& p : m_players)
    {
        if (p.id == id)
            return &p;
    }

    return nullptr;
}

void GameState::draw_player_name(sf::RenderTarget& target, const PlayerSlot& player) const
{
    if (!player.connected)
        return;

    // Skip unnamed / empty cases just in case.
    if (player.nickname.empty())
        return;

    sf::Text nameText(GetContext().fonts->Get(FontID::kMain));
    nameText.setCharacterSize(16);
    nameText.setString(player.nickname);

    // Center the text nicely.
    Utility::CentreOrigin(nameText);

    // White text with black outline so it stays readable.
    nameText.setFillColor(sf::Color::White);
    nameText.setOutlineThickness(2.f);

    if (player.team == GameSettings::Team::Fire)
        nameText.setOutlineColor(sf::Color(200, 80, 40));
    else if (player.team == GameSettings::Team::Water)
        nameText.setOutlineColor(sf::Color(60, 140, 220));
    else
        nameText.setOutlineColor(sf::Color::Black);

    // Position slightly above the player sprite/body.
    const sf::Vector2f pos = player.entity.position();
    nameText.setPosition({ pos.x, pos.y - 65.f });

    target.draw(nameText);
}

sf::Vector2f GameState::get_camera_target() const
{
    const PlayerSlot* localPlayer = nullptr;
    const auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        localPlayer = find_player(0);
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_local_player_id >= 0)
            localPlayer = find_player(m_local_player_id);
        else
            localPlayer = find_player(kPendingLocalPlayerId);
    }
    else
    {
        localPlayer = find_player(0);
    }

    // Fallback to center of the map if no valid player exists yet.
    if (!localPlayer || !localPlayer->connected)
        return { m_world_size.x * 0.5f, m_world_size.y * 0.5f };

    return localPlayer->entity.position();
}

void GameState::update_camera()
{
    sf::Vector2f center = get_camera_target();
    const sf::Vector2f halfSize = m_world_view.getSize() * 0.5f;

    // Clamp camera so it does not show outside the world bounds.
    if (center.x < halfSize.x)
        center.x = halfSize.x;
    if (center.y < halfSize.y)
        center.y = halfSize.y;

    if (center.x > m_world_size.x - halfSize.x)
        center.x = m_world_size.x - halfSize.x;
    if (center.y > m_world_size.y - halfSize.y)
        center.y = m_world_size.y - halfSize.y;

    m_world_view.setCenter(center);
}

GameState::PlayerSlot& GameState::ensure_player_slot(int id)
{
    if (auto* existing = find_player(id))
        return *existing;

    PlayerSlot newSlot;
    newSlot.id = id;
    newSlot.nickname = "Player";
    newSlot.team = GameSettings::Team::Spectator;
    newSlot.connected = false;
    newSlot.entity.set_position(spawn_for_player_id(id));

    // With std::deque, adding a new slot does not force all previous player slots
    // to move like std::vector can. That makes PlayerEntity texture/sprite lifetime safer.
    m_players.push_back(std::move(newSlot));

    return m_players.back();
}

void GameState::apply_team_change_now(PlayerSlot& player, GameSettings::Team newTeam)
{
    // Apply the actual team switch now.
    player.team = newTeam;
    player.has_pending_team_change = false;
    player.pending_team = GameSettings::Team::Spectator;
    player.pending_team_change_timer = sf::Time::Zero;

    // Move to a sensible spawn position when becoming active / switching.
    player.entity.set_position(spawn_for_player_id(player.id));

    if (is_combat_team(newTeam))
        apply_team_visuals(player.entity, newTeam);
    else
        apply_spectator_visuals(player.entity);
}

void GameState::queue_team_change(PlayerSlot& player, GameSettings::Team newTeam)
{
    // Ignore useless requests.
    if (player.team == newTeam && !player.has_pending_team_change)
        return;

    // Store a pending switch. It will be applied by the host
    // after a short delay instead of waiting for death.
    player.has_pending_team_change = true;
    player.pending_team = newTeam;
    player.pending_team_change_timer = sf::Time::Zero;
}

int GameState::count_connected_players_on_team(GameSettings::Team team) const
{
    int count = 0;

    for (const auto& p : m_players)
    {
        if (!p.connected)
            continue;

        if (p.team == team)
            ++count;
    }

    return count;
}

bool GameState::can_join_team(GameSettings::Team team, int ignorePlayerId) const
{
    // Spectator is always allowed.
    if (team == GameSettings::Team::Spectator)
        return true;

    int fireCount = 0;
    int waterCount = 0;

    for (const auto& p : m_players)
    {
        if (!p.connected)
            continue;

        // Ignore the requesting player's current slot while checking limits.
        if (p.id == ignorePlayerId)
            continue;

        if (p.team == GameSettings::Team::Fire)
            ++fireCount;
        else if (p.team == GameSettings::Team::Water)
            ++waterCount;
    }

    const int limit = GetContext().settings->team_limit;

    if (team == GameSettings::Team::Fire)
    {
        if (fireCount >= limit)
            return false;

        // prevent one side from becoming bigger than the other
        if (fireCount > waterCount)
            return false;
    }
    else if (team == GameSettings::Team::Water)
    {
        if (waterCount >= limit)
            return false;

        if (waterCount > fireCount)
            return false;
    }

    return true;
}

GameState::PlayerSlot* GameState::get_local_player_slot()
{
    auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host)
        return find_player(0);

    if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_local_player_id >= 0)
            return find_player(m_local_player_id);

        return find_player(kPendingLocalPlayerId); // temporary slot before host sends the real id
    }

    return find_player(0);
}

void GameState::add_local_kill()
{
    // Count a kill only for the local profile on this machine.
    ++m_local_match_kills;
}

void GameState::add_local_death()
{
    // Count a death only for the local profile on this machine.
    ++m_local_match_deaths;
}

void GameState::save_profile_if_needed()
{
    auto& settings = *GetContext().settings;

    // Keep nickname in sync with the saved profile too.
    settings.profile.nickname = settings.nickname;
    ProfileData::Save(settings.profile);
}

void GameState::finish_match_and_save()
{
    if (m_match_stats_committed)
        return;

    auto& settings = *GetContext().settings;

    // Commit this match into the persistent local profile.
    settings.profile.nickname = settings.nickname;
    settings.profile.matches_played += 1;
    settings.profile.total_kills += m_local_match_kills;
    settings.profile.total_deaths += m_local_match_deaths;

    if (m_local_match_kills > settings.profile.best_match_kills)
        settings.profile.best_match_kills = m_local_match_kills;

    ProfileData::Save(settings.profile);
    m_match_stats_committed = true;
}

LobbyStatePacket GameState::build_running_match_lobby_packet_for_player(int playerId) const
{
    LobbyStatePacket lobby;
    lobby.your_player_id = playerId;
    lobby.match_started = true;   // this is what makes TeamSelectState enter GameState
    lobby.can_spectate = true;

    for (const auto& p : m_players)
    {
        if (!p.connected)
            continue;

        LobbyPlayerState lp;
        lp.id = p.id;
        lp.nickname = p.nickname;
        lp.team = encode_team(p.team);
        lp.connected = p.connected;
        lobby.players.push_back(lp);

        if (p.team == GameSettings::Team::Fire)
            ++lobby.fire_count;
        else if (p.team == GameSettings::Team::Water)
            ++lobby.water_count;
        else
            ++lobby.spectator_count;
    }

    lobby.can_join_fire =
        (lobby.fire_count < GetContext().settings->team_limit &&
            lobby.fire_count <= lobby.water_count);

    lobby.can_join_water =
        (lobby.water_count < GetContext().settings->team_limit &&
            lobby.water_count <= lobby.fire_count);

    return lobby;
}

void GameState::build_pause_gui()
{
    const auto context = GetContext();

    sf::Vector2f viewSize(
        static_cast<float>(context.window->getSize().x),
        static_cast<float>(context.window->getSize().y)
    );

    m_pause_fire_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_pause_fire_button->SetText("Switch to Fire");
    m_pause_fire_button->setPosition({ viewSize.x * 0.5f - 100.f, viewSize.y * 0.42f });
    m_pause_fire_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            PlayerSlot* localPlayer = get_local_player_slot();
            if (!localPlayer)
                return;

            GetContext().sounds->Play(SoundID::kButton);

            if (settings.network_role == GameSettings::NetworkRole::Client)
            {
                if (m_client_session)
                {
                    TeamChangeRequestPacket request;
                    request.requested_team = static_cast<int>(NetTeam::Fire);
                    m_client_session->send_team_change_request(request);
                }
            }
            else
            {
                if (can_switch_local_to_fire())
                    queue_team_change(*localPlayer, GameSettings::Team::Fire);
            }

            m_pause_open = false;
        });

    m_pause_water_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_pause_water_button->SetText("Switch to Water");
    m_pause_water_button->setPosition({ viewSize.x * 0.5f - 100.f, viewSize.y * 0.50f });
    m_pause_water_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            PlayerSlot* localPlayer = get_local_player_slot();
            if (!localPlayer)
                return;

            GetContext().sounds->Play(SoundID::kButton);

            if (settings.network_role == GameSettings::NetworkRole::Client)
            {
                if (m_client_session)
                {
                    TeamChangeRequestPacket request;
                    request.requested_team = static_cast<int>(NetTeam::Water);
                    m_client_session->send_team_change_request(request);
                }
            }
            else
            {
                if (can_switch_local_to_water())
                    queue_team_change(*localPlayer, GameSettings::Team::Water);
            }

            m_pause_open = false;
        });

    m_pause_spectate_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_pause_spectate_button->SetText("Switch to Spectator");
    m_pause_spectate_button->setPosition({ viewSize.x * 0.5f - 100.f, viewSize.y * 0.58f });
    m_pause_spectate_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            PlayerSlot* localPlayer = get_local_player_slot();
            if (!localPlayer)
                return;

            GetContext().sounds->Play(SoundID::kButton);

            if (settings.network_role == GameSettings::NetworkRole::Client)
            {
                if (m_client_session)
                {
                    TeamChangeRequestPacket request;
                    request.requested_team = static_cast<int>(NetTeam::Spectator);
                    m_client_session->send_team_change_request(request);
                }
            }
            else
            {
                queue_team_change(*localPlayer, GameSettings::Team::Spectator);
            }

            m_pause_open = false;
        });

    m_pause_back_to_menu_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_pause_back_to_menu_button->SetText("Back to Menu");
    m_pause_back_to_menu_button->setPosition({ viewSize.x * 0.5f - 100.f, viewSize.y * 0.68f });
    m_pause_back_to_menu_button->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);

            // Leaving mid-match should still save current local progress.
            finish_match_and_save();

            RequestStackClear();
            RequestStackPush(StateID::kMenu);
        });

    m_pause_gui.Pack(m_pause_fire_button);
    m_pause_gui.Pack(m_pause_water_button);
    m_pause_gui.Pack(m_pause_spectate_button);
    m_pause_gui.Pack(m_pause_back_to_menu_button);
}

PlayerInput GameState::build_input_from_keybinds(const PlayerKeybinds& keys, bool& dashPrev)
{
    PlayerInput input;

    // movement keys
    if (sf::Keyboard::isKeyPressed(keys.up))    input.move.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.down))  input.move.y += 1.f;
    if (sf::Keyboard::isKeyPressed(keys.left))  input.move.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.right)) input.move.x += 1.f;

    // preventing faster diagonal movement
    float len = std::sqrt(input.move.x * input.move.x + input.move.y * input.move.y);
    if (len > 0.f)
    {
        input.move.x /= len;
        input.move.y /= len;
    }

    // combat keys
    input.shootHeld = sf::Keyboard::isKeyPressed(keys.shoot);

    bool dashNow = sf::Keyboard::isKeyPressed(keys.dash);
    input.dashPressed = dashNow && !dashPrev;
    dashPrev = dashNow;

    return input;
}

bool GameState::HandleEvent(const sf::Event& event)
{
    if (m_pause_open)
    {
        update_pause_button_states();
        m_pause_gui.HandleEvent(event);
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        // Esc opens/closes the pause menu.
        if (key->scancode == sf::Keyboard::Scancode::Escape)
        {
            m_pause_open = !m_pause_open;
            return false;
        }

        if (m_pause_open)
        {
            // Pause GUI handles buttons now.
            return false;
        }
    }

    return false;
}

void GameState::build_map()
{
    m_walls.clear();

    auto make_wall = [&](float x, float y, float w, float h)
        {
            sf::RectangleShape r;
            r.setSize({ w, h });
            r.setPosition({ x, y });
            r.setFillColor(sf::Color(50, 50, 70));
            m_walls.push_back(r);
        };

    const float W = m_world_size.x;
    const float H = m_world_size.y;

    const float border = 20.f;
    const float thick = 28.f;
    const float thin = 24.f;

    // -------------------------------------------------
    // OUTER BORDERS
    // Keep players and bullets inside the map.
    // -------------------------------------------------
    make_wall(0.f, 0.f, W, border);
    make_wall(0.f, H - border, W, border);
    make_wall(0.f, 0.f, border, H);
    make_wall(W - border, 0.f, border, H);

    // -------------------------------------------------
    // CENTRAL STRUCTURE
    // -------------------------------------------------
    make_wall(W * 0.42f, H * 0.18f, thick, H * 0.22f);
    make_wall(W * 0.55f, H * 0.18f, thick, H * 0.22f);

    make_wall(W * 0.42f, H * 0.60f, thick, H * 0.22f);
    make_wall(W * 0.55f, H * 0.60f, thick, H * 0.22f);

    make_wall(W * 0.46f, H * 0.46f, W * 0.08f, thick);

    // -------------------------------------------------
    // LEFT SIDE COVER
    // -------------------------------------------------
    make_wall(W * 0.18f, H * 0.14f, thick, H * 0.18f);
    make_wall(W * 0.18f, H * 0.52f, thick, H * 0.18f);

    make_wall(W * 0.10f, H * 0.32f, W * 0.12f, thin);
    make_wall(W * 0.12f, H * 0.72f, W * 0.16f, thin);

    make_wall(W * 0.26f, H * 0.24f, W * 0.10f, thin);
    make_wall(W * 0.26f, H * 0.62f, W * 0.10f, thin);

    // -------------------------------------------------
    // RIGHT SIDE COVER
    // -------------------------------------------------
    make_wall(W * 0.80f, H * 0.16f, thick, H * 0.16f);
    make_wall(W * 0.80f, H * 0.56f, thick, H * 0.18f);

    make_wall(W * 0.70f, H * 0.30f, W * 0.14f, thin);
    make_wall(W * 0.72f, H * 0.74f, W * 0.12f, thin);

    make_wall(W * 0.64f, H * 0.18f, W * 0.10f, thin);
    make_wall(W * 0.64f, H * 0.62f, W * 0.10f, thin);

    // -------------------------------------------------
    // TOP LANE OBSTACLES
    // -------------------------------------------------
    make_wall(W * 0.30f, H * 0.10f, W * 0.10f, thin);
    make_wall(W * 0.60f, H * 0.10f, W * 0.10f, thin);

    // -------------------------------------------------
    // BOTTOM LANE OBSTACLES
    // -------------------------------------------------
    make_wall(W * 0.28f, H * 0.86f, W * 0.12f, thin);
    make_wall(W * 0.60f, H * 0.84f, W * 0.12f, thin);

    // -------------------------------------------------
    // MID-LANE SMALL PILLARS
    // -------------------------------------------------
    make_wall(W * 0.32f, H * 0.44f, thick, H * 0.10f);
    make_wall(W * 0.66f, H * 0.44f, thick, H * 0.10f);

    make_wall(W * 0.24f, H * 0.46f, W * 0.06f, thin);
    make_wall(W * 0.70f, H * 0.46f, W * 0.06f, thin);
}

bool GameState::spawn_is_clear(sf::Vector2f p) const
{
    // checking that the respawn point is not inside a wall
    sf::CircleShape probe;
    probe.setRadius(18.f);
    probe.setOrigin({ 18.f, 18.f });
    probe.setPosition(p);

    for (const auto& w : m_walls)
    {
        if (PlayerEntity::circle_rect_intersect(probe, w))
            return false;
    }

    return true;
}

sf::Vector2f GameState::pick_safe_spawn(const PlayerEntity& enemy) const
{
    // choosing a spawn away from the enemy
    const float min_dist = 200.f;
    const float min_dist_sq = min_dist * min_dist;

    std::vector<sf::Vector2f> candidates = m_spawn_points;
    static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(candidates.begin(), candidates.end(), rng);

    // first pass: clear spawn with enough distance
    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;
        if (dist2(sp, enemy.position()) >= min_dist_sq)
            return sp;
    }

    // second pass: farthest clear spawn
    bool foundClear = false;
    sf::Vector2f best = { m_world_size.x * 0.5f, m_world_size.y * 0.5f };
    float best_d2 = -1.f;

    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;

        const float d2 = dist2(sp, enemy.position());
        if (!foundClear || d2 > best_d2)
        {
            foundClear = true;
            best_d2 = d2;
            best = sp;
        }
    }

    // Final fallback in case all spawn points were blocked somehow.
    return best;
}

void GameState::Draw(sf::RenderTarget& target)
{
    // ---------- WORLD VIEW ----------
    target.setView(m_world_view);

    // Large background that covers the whole world.
    sf::RectangleShape bg;
    bg.setSize(m_world_size);
    bg.setFillColor(sf::Color(18, 18, 28));
    target.draw(bg);

    // map and bullets
    for (auto& w : m_walls)
        target.draw(w);

    for (auto& b : m_bullets)
        b.draw(target);

    // draw connected combat players sorted by Y
    std::vector<const PlayerSlot*> drawPlayers;
    for (const auto& p : m_players)
    {
        if (p.connected && is_combat_team(p.team))
            drawPlayers.push_back(&p);
    }

    std::sort(drawPlayers.begin(), drawPlayers.end(),
        [](const PlayerSlot* a, const PlayerSlot* b)
        {
            return a->entity.position().y < b->entity.position().y;
        });

    for (const auto* p : drawPlayers)
    {
        p->entity.draw(target);
        draw_player_name(target, *p);
    }

    // ---------- UI VIEW ----------
    target.setView(target.getDefaultView());

    target.draw(m_hud);

    if (m_pause_open)
    {
        target.draw(m_pause_overlay);
        target.draw(m_pause_title);
        target.draw(m_pause_options);
        target.draw(m_pause_gui);
    }
}

bool GameState::Update(sf::Time dt)
{
    auto& settings = *GetContext().settings;

    if (m_pause_open)
    {
        update_pause_button_states();
    }

    PlayerSlot* localPlayer = nullptr;

    PlayerInput hostInput{};
    PlayerInput clientInput{};

    // Host collects sound events for this frame and sends them to clients.
    std::vector<SoundEventState> frame_sound_events;

    localPlayer = get_local_player_slot();

    // Build local input only for the local player's slot.
    // the multiplayer match keeps running while the in-game menu is open,
    // but the local player should not keep moving / shooting while navigating the menu.
    if (localPlayer && localPlayer->connected && is_combat_team(localPlayer->team))
    {
        PlayerInput localBuilt{};

        if (!m_pause_open)
            localBuilt = build_input_from_keybinds(settings.local_keys, localPlayer->dash_prev);

        if (settings.network_role == GameSettings::NetworkRole::Host)
            hostInput = localBuilt;
        else if (settings.network_role == GameSettings::NetworkRole::Client)
            clientInput = localBuilt;
        else
            hostInput = localBuilt;
    }

    // host receives join info first, then newest remote input
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            // receive joins from any number of clients
            while (true)
            {
                const auto joinInfo = m_host_session->poll_join_info();
                if (!joinInfo.has_value())
                    break;

                PlayerSlot& remotePlayer = ensure_player_slot(joinInfo->player_id);
                remotePlayer.connected = true;
                remotePlayer.nickname = joinInfo->nickname;

                // Validate the client's requested starting team on the host.
                // If the request is not allowed, force spectator instead.
                GameSettings::Team requestedTeam = decode_team(joinInfo->team);

                if (can_join_team(requestedTeam, remotePlayer.id))
                    remotePlayer.team = requestedTeam;
                else
                    remotePlayer.team = GameSettings::Team::Spectator;

                remotePlayer.entity.set_position(spawn_for_player_id(remotePlayer.id));

                if (is_combat_team(remotePlayer.team))
                    apply_team_visuals(remotePlayer.entity, remotePlayer.team);
                else
                    apply_spectator_visuals(remotePlayer.entity);

                // IMPORTANT:
                // If a client joins/reconnects while the match is already running,
                // send them a lobby packet with match_started=true so their
                // TeamSelectState can enter GameState immediately.
                if (m_host_session)
                {
                    LobbyStatePacket runningLobby =
                        build_running_match_lobby_packet_for_player(remotePlayer.id);

                    m_host_session->send_lobby_state_to_player(remotePlayer.id, runningLobby);
                }

                std::cout << "Client joined: " << remotePlayer.nickname
                    << " id=" << remotePlayer.id << "\n";
            }

            // receive newest input from any client
            while (true)
            {
                const auto remote = m_host_session->poll_remote_input();
                if (!remote.has_value())
                    break;

                m_remote_inputs[remote->first] = remote->second;
            }

            // receive team switch requests from clients
            while (true)
            {
                const auto request = m_host_session->poll_team_change_request();
                if (!request.has_value())
                    break;

                const int playerId = request->first;
                PlayerSlot* requestedPlayer = find_player(playerId);

                if (!requestedPlayer || !requestedPlayer->connected)
                    continue;

                const GameSettings::Team requestedTeam = decode_team(request->second.requested_team);

                // host validates whether the team change is allowed
                if (can_join_team(requestedTeam, playerId))
                {
                    queue_team_change(*requestedPlayer, requestedTeam);
                    std::cout << "Queued team change for player id=" << playerId << "\n";
                }
            }

            // clean up disconnected clients
            for (int disconnectedId : m_host_session->consume_disconnected_player_ids())
            {
                if (PlayerSlot* disconnectedPlayer = find_player(disconnectedId))
                {
                    disconnectedPlayer->connected = false;
                    disconnectedPlayer->team = GameSettings::Team::Spectator;

                    // Clear any queued team switch from the old connection.
                    disconnectedPlayer->has_pending_team_change = false;
                    disconnectedPlayer->pending_team = GameSettings::Team::Spectator;

                    disconnectedPlayer->nickname += " (left)";
                }

                m_remote_inputs.erase(disconnectedId);

                std::cout << "Client disconnected: id=" << disconnectedId << "\n";
            }

            // keep shared settings counts updated on host too
            settings.latest_fire_count = 0;
            settings.latest_water_count = 0;
            settings.latest_spectator_count = 0;

            for (const auto& p : m_players)
            {
                if (!p.connected)
                    continue;

                if (p.team == GameSettings::Team::Fire)
                    ++settings.latest_fire_count;
                else if (p.team == GameSettings::Team::Water)
                    ++settings.latest_water_count;
                else
                    ++settings.latest_spectator_count;
            }
        }
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_client_session &&
            localPlayer &&
            localPlayer->connected &&
            is_combat_team(localPlayer->team))
        {
            m_client_session->send_local_input(clientInput);
        }
    }

    // offline fallback
    if (settings.network_role == GameSettings::NetworkRole::None)
    {
        PlayerSlot* offlineHost = find_player(0);
        if (offlineHost && offlineHost->connected && is_combat_team(offlineHost->team))
            hostInput = build_input_from_keybinds(settings.local_keys, offlineHost->dash_prev);
    }

    // Host/offline applies pending team switches after a short delay.
    // This works for alive players and spectators too.
    if (settings.network_role != GameSettings::NetworkRole::Client)
    {
        for (auto& p : m_players)
        {
            if (!p.connected || !p.has_pending_team_change)
                continue;

            p.pending_team_change_timer += dt;

            if (p.pending_team_change_timer >= kTeamSwitchDelay)
            {
                // Re-check the team rules at the moment of applying.
                // This keeps balancing correct even if counts changed meanwhile.
                if (can_join_team(p.pending_team, p.id))
                {
                    apply_team_change_now(p, p.pending_team);
                }
            }
        }
    }

    // Update players.
    // Host/offline simulates all active combat players.
    // Client simulates only its own local player.
    // Remote players on the client are driven by host snapshots.
    for (auto& p : m_players)
    {
        if (!p.connected || !is_combat_team(p.team))
            continue;

        if (settings.network_role == GameSettings::NetworkRole::Host)
        {
            PlayerInput input{};

            if (p.id == 0)
            {
                input = hostInput;
            }
            else
            {
                auto found = m_remote_inputs.find(p.id);
                if (found != m_remote_inputs.end())
                    input = found->second;
            }

            p.entity.update(dt, input, m_walls);
        }
        else if (settings.network_role == GameSettings::NetworkRole::Client)
        {
            // Only the local player runs full gameplay update on the client.
            if (localPlayer && p.id == localPlayer->id)
            {
                p.entity.update(dt, clientInput, m_walls);

                // Play the local cast sound immediately on this machine.
                // This makes the client's own shooting feel responsive
                // instead of waiting for the host snapshot to come back.
                if (p.entity.consume_shot_event())
                {
                    if (p.team == GameSettings::Team::Fire)
                    {
                        GetContext().sounds->Play(SoundID::kFireSpell);
                    }
                    else if (p.team == GameSettings::Team::Water)
                    {
                        GetContext().sounds->Play(SoundID::kWaterSpell);
                    }
                }
            }
            else
            {
                // Remote players on the client do not run gameplay simulation.
                // They only play the visual animation state chosen by the host.
                p.entity.apply_network_visual_state(
                    p.replicated_anim_state,
                    p.replicated_dir,
                    dt
                );
            }
        }
        else
        {
            // Offline fallback
            if (p.id == 0)
                p.entity.update(dt, hostInput, m_walls);
        }
    }

    // Client keeps bullets moving locally between host snapshots.
    // This makes projectile motion look smoother instead of stepping
    // only when a new world state packet arrives.
    if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        for (auto& b : m_bullets)
            b.update(dt);
    }

    // host/offline simulates bullets and combat
    if (settings.network_role != GameSettings::NetworkRole::Client)
    {
        // spawn bullets from all active players
        for (auto& p : m_players)
        {
            if (!p.connected || !is_combat_team(p.team))
                continue;

            if (!p.entity.consume_shot_event())
                continue;

            if (p.team == GameSettings::Team::Fire)
            {
                GetContext().sounds->Play(SoundID::kFireSpell);

                // Tell clients which player caused this sound.
                frame_sound_events.push_back({
                    static_cast<int>(SoundID::kFireSpell),
                    p.id
                    });

                m_bullets.emplace_back(
                    m_next_bullet_id++,
                    p.entity.get_projectile_spawn_point(6.f),
                    p.entity.facing_dir(),
                    p.id,
                    Bullet::SpellType::Fire
                );
            }
            else if (p.team == GameSettings::Team::Water)
            {
                GetContext().sounds->Play(SoundID::kWaterSpell);

                // Tell clients which player caused this sound.
                frame_sound_events.push_back({
                    static_cast<int>(SoundID::kWaterSpell),
                    p.id
                    });

                m_bullets.emplace_back(
                    m_next_bullet_id++,
                    p.entity.get_projectile_spawn_point(6.f),
                    p.entity.facing_dir(),
                    p.id,
                    Bullet::SpellType::Water
                );
            }
        }

        // move bullets
        for (auto& b : m_bullets)
            b.update(dt);

        // bullet vs wall collision
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            const auto bb = b.shape().getGlobalBounds();

            for (const auto& w : m_walls)
            {
                if (bb.findIntersection(w.getGlobalBounds()).has_value())
                {
                    b.kill();
                    break;
                }
            }
        }

        // bullet vs players
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            PlayerSlot* shooter = find_player(b.owner());
            if (!shooter || !shooter->connected)
                continue;

            const sf::Vector2f bp = b.shape().getPosition();
            const float br = b.shape().getRadius();

            for (auto& targetPlayer : m_players)
            {
                if (!targetPlayer.connected || !is_combat_team(targetPlayer.team))
                    continue;

                if (targetPlayer.id == b.owner())
                    continue;

                if (targetPlayer.entity.is_invulnerable())
                    continue;

                const bool sameTeam = (shooter->team == targetPlayer.team);

                // preventing team kills
                if (!sameTeam && targetPlayer.entity.bullet_hits_hurtbox(bp, br))
                {
                    b.kill();

                    if (shooter->team == GameSettings::Team::Fire)
                    {
                        GetContext().sounds->Play(SoundID::kFireHit);

                        // Hit sound is also tagged with the player who caused it.
                        frame_sound_events.push_back({
                            static_cast<int>(SoundID::kFireHit),
                            shooter->id
                            });

                        ++m_fire_kills;
                    }
                    else if (shooter->team == GameSettings::Team::Water)
                    {
                        GetContext().sounds->Play(SoundID::kWaterHit);

                        // Hit sound is also tagged with the player who caused it.
                        frame_sound_events.push_back({
                            static_cast<int>(SoundID::kWaterHit),
                            shooter->id
                            });

                        ++m_water_kills;
                    }

                    // Update persistent local stats only for the local player on this PC.
                    if (settings.network_role == GameSettings::NetworkRole::Host)
                    {
                        if (shooter->id == 0)
                            add_local_kill();

                        if (targetPlayer.id == 0)
                            add_local_death();
                    }
                    else
                    {
                        // Offline mode: local player is id 0.
                        if (shooter->id == 0)
                            add_local_kill();

                        if (targetPlayer.id == 0)
                            add_local_death();
                    }

                    targetPlayer.entity.respawn(pick_safe_spawn(shooter->entity));
                    break;
                }
            }
        }

        // remove dead bullets
        m_bullets.erase(
            std::remove_if(
                m_bullets.begin(),
                m_bullets.end(),
                [](const Bullet& b) { return b.is_dead(); }
            ),
            m_bullets.end()
        );
    }

    // host sends world state using player list snapshot
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            // build common snapshot once
            WorldStatePacket baseState;
            baseState.fire_kills = m_fire_kills;
            baseState.water_kills = m_water_kills;

            // counting connected players by team for UI
            for (const auto& p : m_players)
            {
                if (!p.connected)
                    continue;

                if (p.team == GameSettings::Team::Fire)
                    ++baseState.fire_count;
                else if (p.team == GameSettings::Team::Water)
                    ++baseState.water_count;
                else
                    ++baseState.spectator_count;
            }

            // sending all player slots
            for (const auto& p : m_players)
            {
                PlayerNetState netPlayer;
                netPlayer.id = p.id;
                netPlayer.nickname = p.nickname;
                netPlayer.pos = p.entity.position();
                netPlayer.dir = p.entity.facing_dir();
                netPlayer.team = encode_team(p.team);
                netPlayer.connected = p.connected;

                // Send pending team-switch info too, so clients can display it
                netPlayer.has_pending_team_change = p.has_pending_team_change;
                netPlayer.pending_team = encode_team(p.pending_team);

                // tell clients which animation this player is currently using
                netPlayer.anim_state = p.entity.get_net_anim_state();

                // Send respawn blink / spawn protection too,
                // so clients can render invulnerability correctly.
                netPlayer.invulnerable = p.entity.is_invulnerable();
                netPlayer.invulnerable_time_seconds = p.entity.invulnerable_elapsed().asSeconds();

                baseState.players.push_back(netPlayer);
            }

            // sending bullets
            for (const auto& b : m_bullets)
            {
                if (b.is_dead()) continue;

                BulletState bs;
                bs.bullet_id = b.bullet_id();
                bs.pos = b.shape().getPosition();
                bs.dir = b.direction();
                bs.owner = b.owner();

                PlayerSlot* ownerPlayer = find_player(b.owner());
                if (ownerPlayer && ownerPlayer->team == GameSettings::Team::Water)
                    bs.spell = 1;
                else
                    bs.spell = 0;

                baseState.bullets.push_back(bs);
            }

            // Send all sound events that happened during this host frame.
            baseState.sound_events = frame_sound_events;

            // send a personalized world state to each remote player
            for (const auto& p : m_players)
            {
                if (!p.connected || p.id == 0)
                    continue;

                WorldStatePacket stateForClient = baseState;
                stateForClient.your_player_id = p.id;
                m_host_session->send_world_state_to_player(p.id, stateForClient);
            }
        }
    }

    // client receives world state using player list snapshot
    if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_client_session)
        {
            while (true)
            {
                const auto state = m_client_session->poll_world_state();
                if (!state.has_value())
                    break;

                m_latest_world_state = *state;
            }

            if (m_latest_world_state.has_value())
            {
                // learn which player slot belongs to this client
                if (m_latest_world_state->your_player_id >= 0)
                {
                    const int assignedId = m_latest_world_state->your_player_id;

                    // Once the host tells us our real id, disable the temporary
                    // local placeholder slot so it can never conflict with the
                    // real replicated player entry.
                    if (m_local_player_id < 0)
                    {
                        if (PlayerSlot* tempSlot = find_player(kPendingLocalPlayerId))
                        {
                            tempSlot->connected = false;
                            tempSlot->team = GameSettings::Team::Spectator;
                        }
                    }

                    m_local_player_id = assignedId;
                }

                m_fire_kills = m_latest_world_state->fire_kills;
                m_water_kills = m_latest_world_state->water_kills;

                settings.latest_fire_count = m_latest_world_state->fire_count;
                settings.latest_water_count = m_latest_world_state->water_count;
                settings.latest_spectator_count = m_latest_world_state->spectator_count;

                // applying all player snapshots
                for (const auto& netPlayer : m_latest_world_state->players)
                {
                    PlayerSlot* localSlot = &ensure_player_slot(netPlayer.id);

                    const auto newTeam = decode_team(netPlayer.team);
                    const bool teamChanged = (newTeam != localSlot->team);

                    localSlot->nickname = netPlayer.nickname;
                    localSlot->team = newTeam;
                    localSlot->connected = netPlayer.connected;

                    // Host is authoritative for remote positions.
                    localSlot->entity.set_position(netPlayer.pos);
                    localSlot->entity.set_facing_dir(netPlayer.dir);

                    // Store replicated animation info from the host.
                    localSlot->replicated_anim_state = netPlayer.anim_state;
                    localSlot->replicated_dir = netPlayer.dir;

                    // Detect fresh respawn for the local client player.
                    // When the host flips us into invulnerable state,
                    // that means we have just died and respawned.
                    const bool wasInvulnerable = localSlot->replicated_invulnerable;
                    localSlot->replicated_invulnerable = netPlayer.invulnerable;

                    if (m_local_player_id >= 0 &&
                        netPlayer.id == m_local_player_id &&
                        netPlayer.invulnerable &&
                        !wasInvulnerable)
                    {
                        add_local_death();
                    }

                    // Copy host invulnerability state too,
                    // so respawn blinking is visible on clients.
                    localSlot->entity.set_invulnerability_state(
                        netPlayer.invulnerable,
                        sf::seconds(netPlayer.invulnerable_time_seconds)
                    );

                    // Apply pending switch state from host snapshot too.
                    localSlot->has_pending_team_change = netPlayer.has_pending_team_change;
                    localSlot->pending_team = decode_team(netPlayer.pending_team);

                    if (teamChanged)
                    {
                        if (is_combat_team(localSlot->team))
                            apply_team_visuals(localSlot->entity, localSlot->team);
                        else
                            apply_spectator_visuals(localSlot->entity);
                    }
                }

                // Smooth bullet sync:
                // Do NOT destroy and recreate all bullets every packet.
                // Instead, update existing bullets by bullet_id, create only
                // missing ones, and remove bullets that no longer exist on host.
                std::vector<int> seen_bullet_ids;
                seen_bullet_ids.reserve(m_latest_world_state->bullets.size());

                for (const auto& netBullet : m_latest_world_state->bullets)
                {
                    seen_bullet_ids.push_back(netBullet.bullet_id);

                    Bullet* localBullet = nullptr;

                    for (auto& existing : m_bullets)
                    {
                        if (existing.bullet_id() == netBullet.bullet_id)
                        {
                            localBullet = &existing;
                            break;
                        }
                    }

                    if (localBullet)
                    {
                        // Correct the existing bullet to the newest host snapshot.
                        // We keep the bullet object alive so its animation and motion
                        // do not restart every packet.
                        localBullet->set_position(netBullet.pos);
                        localBullet->set_direction(netBullet.dir);
                    }
                    else
                    {
                        Bullet::SpellType spell =
                            (netBullet.spell == 0)
                            ? Bullet::SpellType::Fire
                            : Bullet::SpellType::Water;

                        m_bullets.emplace_back(
                            netBullet.bullet_id,
                            netBullet.pos,
                            netBullet.dir,
                            netBullet.owner,
                            spell
                        );
                    }
                }

                // Remove bullets that are no longer present in the host snapshot.
                m_bullets.erase(
                    std::remove_if(
                        m_bullets.begin(),
                        m_bullets.end(),
                        [&](const Bullet& bullet)
                        {
                            return std::find(
                                seen_bullet_ids.begin(),
                                seen_bullet_ids.end(),
                                bullet.bullet_id()
                            ) == seen_bullet_ids.end();
                        }
                    ),
                    m_bullets.end()
                );

                // Play sound events that happened on the host this frame.
                // Ignore our own replicated spell sound because we already
                // played it immediately when the local cast released.
                for (const auto& s : m_latest_world_state->sound_events)
                {
                    const SoundID sound = static_cast<SoundID>(s.sound_id);

                    const bool isLocalSource =
                        (m_local_player_id >= 0 && s.source_player_id == m_local_player_id);

                    const bool isSpellSound =
                        (sound == SoundID::kFireSpell || sound == SoundID::kWaterSpell);

                    // If the host says this hit sound was caused by our local player,
                    // count it as a local kill in the persistent profile stats.
                    if ((sound == SoundID::kFireHit || sound == SoundID::kWaterHit) &&
                        isLocalSource)
                    {
                        add_local_kill();
                    }

                    // Ignore our own replicated cast sound because we already
                    // played it immediately when the local cast released.
                    if (isLocalSource && isSpellSound)
                        continue;

                    GetContext().sounds->Play(sound);
                }
            }
        }
    }

    // HUD from player list
    std::ostringstream ss;

    std::string localName = "Player";
    std::string localTeam = "Spectator";
    std::string otherSummary = "None";

    PlayerSlot* hudLocalPlayer = nullptr;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        hudLocalPlayer = find_player(0);
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_local_player_id >= 0)
            hudLocalPlayer = find_player(m_local_player_id);
        else
            hudLocalPlayer = find_player(kPendingLocalPlayerId); // temporary local slot before real id arrives
    }
    else
    {
        hudLocalPlayer = find_player(0);
    }

    std::string pendingText = "None";

    if (hudLocalPlayer)
    {
        localName = hudLocalPlayer->nickname;
        localTeam = team_to_string(hudLocalPlayer->team);

        if (hudLocalPlayer->has_pending_team_change)
            pendingText = team_to_string(hudLocalPlayer->pending_team);
    }

    std::vector<std::string> others;
    for (const auto& p : m_players)
    {
        if (!p.connected)
            continue;

        // Do not show the local player inside the "Others" list.
        if (hudLocalPlayer && p.id == hudLocalPlayer->id)
            continue;

        others.push_back(p.nickname + " [" + team_to_string(p.team) + "]");
    }

    if (!others.empty())
    {
        otherSummary.clear();
        for (std::size_t i = 0; i < others.size(); ++i)
        {
            if (i > 0) otherSummary += ", ";
            otherSummary += others[i];
        }
    }

    ss << "Fire: " << m_fire_kills
        << "   |   Water: " << m_water_kills
        << "   |   First to " << m_kills_to_win << "\n"
        << "You: " << localName << " [" << localTeam << "]";

    if (pendingText != "None")
    {
        ss << "\nPending: " << pendingText;
    }

    ss << "\nOthers: " << otherSummary;

    m_hud.setString(ss.str());

    m_pause_options.setString(
        "Choose a team switch option:\n"
        "Disabled buttons mean the switch is not allowed right now."
    );

    update_camera();

    // win condition
    if (m_fire_kills >= m_kills_to_win || m_water_kills >= m_kills_to_win)
    {
        if (m_fire_kills >= m_kills_to_win)
            settings.last_winner_team = GameSettings::Team::Fire;
        else
            settings.last_winner_team = GameSettings::Team::Water;

        // Save this match into the local persistent profile before leaving.
        finish_match_and_save();

        RequestStackClear();
        RequestStackPush(StateID::kGameOver);
        return false;
    }

    return true;
}

bool GameState::can_switch_local_to_fire() const
{
    const PlayerSlot* localPlayer = nullptr;

    auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        localPlayer = find_player(0);
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_local_player_id >= 0)
            localPlayer = find_player(m_local_player_id);
        else
            localPlayer = find_player(kPendingLocalPlayerId);
    }
    else
    {
        localPlayer = find_player(0);
    }

    if (!localPlayer || !localPlayer->connected)
        return false;

    if (localPlayer->team == GameSettings::Team::Fire)
        return false;

    return can_join_team(GameSettings::Team::Fire, localPlayer->id);
}

bool GameState::can_switch_local_to_water() const
{
    const PlayerSlot* localPlayer = nullptr;

    auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        localPlayer = find_player(0);
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_local_player_id >= 0)
            localPlayer = find_player(m_local_player_id);
        else
            localPlayer = find_player(kPendingLocalPlayerId);
    }
    else
    {
        localPlayer = find_player(0);
    }

    if (!localPlayer || !localPlayer->connected)
        return false;

    if (localPlayer->team == GameSettings::Team::Water)
        return false;

    return can_join_team(GameSettings::Team::Water, localPlayer->id);
}

void GameState::update_pause_button_states()
{
    if (m_pause_fire_button)
        m_pause_fire_button->SetEnabled(can_switch_local_to_fire());

    if (m_pause_water_button)
        m_pause_water_button->SetEnabled(can_switch_local_to_water());

    if (m_pause_spectate_button)
        m_pause_spectate_button->SetEnabled(true);

    if (m_pause_back_to_menu_button)
        m_pause_back_to_menu_button->SetEnabled(true);
}

void GameState::rebuild_pause_layout(sf::Vector2u new_size)
{
    const sf::Vector2f viewSize(static_cast<float>(new_size.x), static_cast<float>(new_size.y));

    m_pause_overlay.setSize(viewSize);

    Utility::CentreOrigin(m_pause_title);
    m_pause_title.setPosition({ viewSize.x * 0.5f, viewSize.y * 0.18f });

    Utility::CentreOrigin(m_pause_options);
    m_pause_options.setPosition({ viewSize.x * 0.5f, viewSize.y * 0.30f });

    const float buttonX = viewSize.x * 0.5f - 100.f;
    const float startY = viewSize.y * 0.43f;
    const float gap = viewSize.y * 0.08f;

    if (m_pause_fire_button)
        m_pause_fire_button->setPosition({ buttonX, startY });

    if (m_pause_water_button)
        m_pause_water_button->setPosition({ buttonX, startY + gap });

    if (m_pause_spectate_button)
        m_pause_spectate_button->setPosition({ buttonX, startY + gap * 2.f });

    if (m_pause_back_to_menu_button)
        m_pause_back_to_menu_button->setPosition({ buttonX, startY + gap * 3.2f });

    m_world_view.setSize(viewSize);
}

void GameState::OnResize(sf::Vector2u new_size)
{
    rebuild_pause_layout(new_size);
    update_camera();
}

GameState::~GameState()
{
    // Save local progress if the player leaves before normal match end.
    finish_match_and_save();

    // When leaving the match, close host/client networking cleanly.
    if (GetContext().network)
        GetContext().network->disconnect();
}