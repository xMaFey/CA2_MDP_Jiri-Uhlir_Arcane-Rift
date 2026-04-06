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

namespace
{
    // checking if player is an active player in the game
    bool is_combat_team(GameSettings::Team t)
    {
        return t == GameSettings::Team::Fire || t == GameSettings::Team::Water;
    }

    // applying correct wizard color and animation for team
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

    // converting network team id back to local team enum
    GameSettings::Team decode_team(int t)
    {
        if (t == static_cast<int>(NetTeam::Fire)) return GameSettings::Team::Fire;
        if (t == static_cast<int>(NetTeam::Water)) return GameSettings::Team::Water;
        return GameSettings::Team::Spectator;
    }

    // converting local team enum to network team id
    int encode_team(GameSettings::Team t)
    {
        if (t == GameSettings::Team::Fire) return static_cast<int>(NetTeam::Fire);
        if (t == GameSettings::Team::Water) return static_cast<int>(NetTeam::Water);
        return static_cast<int>(NetTeam::Spectator);
    }

    // squared distance helper for spawn checks
    float dist2(sf::Vector2f a, sf::Vector2f b)
    {
        sf::Vector2f d = a - b;
        return d.x * d.x + d.y * d.y;
    }
}

GameState::GameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_hud(context.fonts->Get(FontID::kMain))
{
    auto& settings = *GetContext().settings;

    GetContext().music->Stop();

    // starting host or client session
    if (GetContext().network)
    {
        auto& network = *GetContext().network;

        if (settings.network_role == GameSettings::NetworkRole::Host)
        {
            m_host_session = std::make_unique<HostSession>(network);

            const bool ok = m_host_session->start(settings.server_port);
            if (ok)
                std::cout << "Host started on port " << settings.server_port << "\n";
            else
                std::cout << "Host failed\n";
        }
        else if (settings.network_role == GameSettings::NetworkRole::Client)
        {
            m_client_session = std::make_unique<ClientSession>(network);

            const auto ip = sf::IpAddress::resolve(settings.server_ip);

            bool ok = false;
            if (ip.has_value())
                ok = m_client_session->connect(*ip, settings.server_port);

            std::cout << (ok ? "Client connected\n" : "Client failed to connect\n");
        }
        else
        {
            network.disconnect();
        }
    }

    build_map();

    // default spawn positions for the two network sides
    m_p1.set_position({ 260.f, 360.f });
    m_p2.set_position({ 1020.f, 360.f });

    // assigning local chosen team to correct network side
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        m_p1_team = settings.chosen_team;
        m_p2_team = GameSettings::Team::Spectator; // remote side unknown for now
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        m_p1_team = GameSettings::Team::Spectator; // host side will come from server
        m_p2_team = settings.chosen_team;
    }
    else
    {
        // offline fallback
        m_p1_team = GameSettings::Team::Fire;
        m_p2_team = GameSettings::Team::Water;
    }

    // loading correct visuals only for active fighters
    if (is_combat_team(m_p1_team))
        apply_team_visuals(m_p1, m_p1_team);

    if (is_combat_team(m_p2_team))
        apply_team_visuals(m_p2, m_p2_team);

    m_hud.setCharacterSize(22);
    m_hud.setPosition({ 14.f, 10.f });

    // possible respawn positions
    m_spawn_points =
    {
        {150.f, 140.f},
        {150.f, 620.f},
        {1130.f, 140.f},
        {1130.f, 620.f},
        {640.f, 360.f}
    };
}

PlayerInput GameState::build_input_from_keybinds(const PlayerKeybinds& keys, bool& dashPrev)
{
    PlayerInput input;

    // reading movement keys
    if (sf::Keyboard::isKeyPressed(keys.up))    input.move.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.down))  input.move.y += 1.f;
    if (sf::Keyboard::isKeyPressed(keys.left))  input.move.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.right)) input.move.x += 1.f;

    // normalising diagonal movement
    float len = std::sqrt(input.move.x * input.move.x + input.move.y * input.move.y);
    if (len > 0.f)
    {
        input.move.x /= len;
        input.move.y /= len;
    }

    // reading combat input
    input.shootHeld = sf::Keyboard::isKeyPressed(keys.shoot);

    bool dashNow = sf::Keyboard::isKeyPressed(keys.dash);
    input.dashPressed = dashNow && !dashPrev;
    dashPrev = dashNow;

    return input;
}

bool GameState::HandleEvent(const sf::Event& event)
{
    return true;
}

void GameState::build_map()
{
    m_walls.clear();

    auto make_wall = [&](float x, float y, float w, float h)
        {
            sf::RectangleShape r;
            r.setSize({ w, h });
            r.setPosition({ x, y });
            r.setFillColor(sf::Color(40, 40, 55));
            m_walls.push_back(r);
        };

    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);

    // border walls
    make_wall(0.f, 0.f, W, 20.f);
    make_wall(0.f, H - 20.f, W, 20.f);
    make_wall(0.f, 0.f, 20.f, H);
    make_wall(W - 20.f, 0.f, 20.f, H);

    // map walls
    make_wall(W * 0.20f, H * 0.18f, 24.f, H * 0.52f);
    make_wall(W * 0.35f, H * 0.30f, W * 0.30f, 24.f);
    make_wall(W * 0.55f, H * 0.18f, 24.f, H * 0.55f);
    make_wall(W * 0.25f, H * 0.72f, W * 0.35f, 24.f);
    make_wall(W * 0.72f, H * 0.40f, 24.f, H * 0.40f);
}

bool GameState::spawn_is_clear(sf::Vector2f p) const
{
    // checking that spawn is not inside a wall
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
    // choosing a respawn far from enemy
    const float min_dist = 200.f;
    const float min_dist_sq = min_dist * min_dist;

    std::vector<sf::Vector2f> candidates = m_spawn_points;
    static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(candidates.begin(), candidates.end(), rng);

    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;
        if (dist2(sp, enemy.position()) >= min_dist_sq)
            return sp;
    }

    sf::Vector2f best = candidates.front();
    float best_d2 = -1.f;

    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;

        const float d2 = dist2(sp, enemy.position());
        if (d2 > best_d2)
        {
            best_d2 = d2;
            best = sp;
        }
    }

    return best;
}

void GameState::Draw(sf::RenderTarget& target)
{
    // drawing background
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(m_window.getSize()));
    bg.setFillColor(sf::Color(18, 18, 28));
    target.draw(bg);

    // drawing map and bullets
    for (auto& w : m_walls) target.draw(w);
    for (auto& b : m_bullets) b.draw(target);

    const bool p1Active = is_combat_team(m_p1_team);
    const bool p2Active = is_combat_team(m_p2_team);

    // drawing only active players
    if (p1Active && p2Active)
    {
        if (m_p1.position().y < m_p2.position().y)
        {
            m_p1.draw(target);
            m_p2.draw(target);
        }
        else
        {
            m_p2.draw(target);
            m_p1.draw(target);
        }
    }
    else if (p1Active)
    {
        m_p1.draw(target);
    }
    else if (p2Active)
    {
        m_p2.draw(target);
    }

    target.draw(m_hud);
}

bool GameState::Update(sf::Time dt)
{
    const auto& settings = *GetContext().settings;

    PlayerInput p1Input{};
    PlayerInput p2Input{};

    // building local input for the side controlled on this machine
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (is_combat_team(m_p1_team))
            p1Input = build_input_from_keybinds(settings.local_keys, m_p1_dash_prev);
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (is_combat_team(m_p2_team))
            p2Input = build_input_from_keybinds(settings.local_keys, m_p2_dash_prev);
    }
    else
    {
        // offline fallback
        if (is_combat_team(m_p1_team))
            p1Input = build_input_from_keybinds(settings.local_keys, m_p1_dash_prev);
        if (is_combat_team(m_p2_team))
            p2Input = build_input_from_keybinds(settings.local_keys, m_p2_dash_prev);
    }

    // host receives remote client input
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            while (true)
            {
                const auto remote = m_host_session->poll_remote_input();
                if (!remote.has_value())
                    break;

                m_remote_input = *remote;
            }
        }

        // applying newest client input to client-side player
        if (m_remote_input.has_value() && is_combat_team(m_p2_team))
        {
            p2Input = *m_remote_input;
        }
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        // client sends its own local input to host
        if (m_client_session && is_combat_team(m_p2_team))
        {
            m_client_session->send_local_input(p2Input);
        }
    }

    // updating active players only
    if (is_combat_team(m_p1_team))
        m_p1.update(dt, p1Input, m_walls);

    if (is_combat_team(m_p2_team))
        m_p2.update(dt, p2Input, m_walls);

    // host simulates bullets and combat
    if (settings.network_role != GameSettings::NetworkRole::Client)
    {
        // spawning bullets using actual team spell type
        if (is_combat_team(m_p1_team) && m_p1.consume_shot_event())
        {
            if (m_p1_team == GameSettings::Team::Fire)
            {
                GetContext().sounds->Play(SoundID::kFireSpell);
                m_bullets.emplace_back(
                    m_p1.get_projectile_spawn_point(6.f),
                    m_p1.facing_dir(),
                    1,
                    Bullet::SpellType::Fire
                );
            }
            else if (m_p1_team == GameSettings::Team::Water)
            {
                GetContext().sounds->Play(SoundID::kWaterSpell);
                m_bullets.emplace_back(
                    m_p1.get_projectile_spawn_point(6.f),
                    m_p1.facing_dir(),
                    1,
                    Bullet::SpellType::Water
                );
            }
        }

        if (is_combat_team(m_p2_team) && m_p2.consume_shot_event())
        {
            if (m_p2_team == GameSettings::Team::Fire)
            {
                GetContext().sounds->Play(SoundID::kFireSpell);
                m_bullets.emplace_back(
                    m_p2.get_projectile_spawn_point(6.f),
                    m_p2.facing_dir(),
                    2,
                    Bullet::SpellType::Fire
                );
            }
            else if (m_p2_team == GameSettings::Team::Water)
            {
                GetContext().sounds->Play(SoundID::kWaterSpell);
                m_bullets.emplace_back(
                    m_p2.get_projectile_spawn_point(6.f),
                    m_p2.facing_dir(),
                    2,
                    Bullet::SpellType::Water
                );
            }
        }

        // moving bullets
        for (auto& b : m_bullets)
            b.update(dt);

        // checking collision for bullets against walls
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

        // checking bullet hits and preventing team kills
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            const sf::Vector2f bp = b.shape().getPosition();
            const float br = b.shape().getRadius();

            // bullet from host side hitting client side
            if (b.owner() == 1 && is_combat_team(m_p2_team) && !m_p2.is_invulnerable())
            {
                const bool sameTeam = (m_p1_team == m_p2_team);

                if (!sameTeam && m_p2.bullet_hits_hurtbox(bp, br))
                {
                    b.kill();

                    if (m_p1_team == GameSettings::Team::Fire)
                    {
                        GetContext().sounds->Play(SoundID::kFireHit);
                        ++m_fire_kills;
                    }
                    else if (m_p1_team == GameSettings::Team::Water)
                    {
                        GetContext().sounds->Play(SoundID::kWaterHit);
                        ++m_water_kills;
                    }

                    m_p2.respawn(pick_safe_spawn(m_p1));
                }
            }

            // bullet from client side hitting host side
            else if (b.owner() == 2 && is_combat_team(m_p1_team) && !m_p1.is_invulnerable())
            {
                const bool sameTeam = (m_p1_team == m_p2_team);

                if (!sameTeam && m_p1.bullet_hits_hurtbox(bp, br))
                {
                    b.kill();

                    if (m_p2_team == GameSettings::Team::Fire)
                    {
                        GetContext().sounds->Play(SoundID::kFireHit);
                        ++m_fire_kills;
                    }
                    else if (m_p2_team == GameSettings::Team::Water)
                    {
                        GetContext().sounds->Play(SoundID::kWaterHit);
                        ++m_water_kills;
                    }

                    m_p1.respawn(pick_safe_spawn(m_p2));
                }
            }
        }

        // removing dead bullets
        m_bullets.erase(
            std::remove_if(m_bullets.begin(), m_bullets.end(),
                [](const Bullet& b) { return b.is_dead(); }),
            m_bullets.end()
        );
    }

    // host sends world state to client
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            WorldStatePacket state;

            state.p1_pos = m_p1.position();
            state.p2_pos = m_p2.position();
            state.p1_dir = m_p1.facing_dir();
            state.p2_dir = m_p2.facing_dir();
            state.p1_team = encode_team(m_p1_team);
            state.p2_team = encode_team(m_p2_team);
            state.fire_kills = m_fire_kills;
            state.water_kills = m_water_kills;

            // sending bullets to client
            for (const auto& b : m_bullets)
            {
                if (b.is_dead()) continue;

                BulletState bs;
                bs.pos = b.shape().getPosition();
                bs.dir = b.direction();
                bs.owner = b.owner();
                bs.spell = (b.owner() == 1 && m_p1_team == GameSettings::Team::Water) ||
                    (b.owner() == 2 && m_p2_team == GameSettings::Team::Water)
                    ? 1 : 0;

                state.bullets.push_back(bs);
            }

            m_host_session->send_world_state(state);
        }
    }

    // client receives latest world state from host
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
                const auto newP1Team = decode_team(m_latest_world_state->p1_team);
                const auto newP2Team = decode_team(m_latest_world_state->p2_team);

                if (newP1Team != m_p1_team)
                {
                    m_p1_team = newP1Team;
                    if (is_combat_team(m_p1_team))
                        apply_team_visuals(m_p1, m_p1_team);
                }

                if (newP2Team != m_p2_team)
                {
                    m_p2_team = newP2Team;
                    if (is_combat_team(m_p2_team))
                        apply_team_visuals(m_p2, m_p2_team);
                }

                m_p1.set_position(m_latest_world_state->p1_pos);
                m_p2.set_position(m_latest_world_state->p2_pos);

                m_p1.set_facing_dir(m_latest_world_state->p1_dir);
                m_p2.set_facing_dir(m_latest_world_state->p2_dir);

                m_fire_kills = m_latest_world_state->fire_kills;
                m_water_kills = m_latest_world_state->water_kills;

                m_bullets.clear();

                for (const auto& b : m_latest_world_state->bullets)
                {
                    Bullet::SpellType spell =
                        (b.spell == 0) ? Bullet::SpellType::Fire : Bullet::SpellType::Water;

                    m_bullets.emplace_back(b.pos, b.dir, b.owner, spell);
                }
            }
        }
    }

    // updating HUD text
    std::ostringstream ss;
    ss << "Fire: " << m_fire_kills
        << "   |   Water: " << m_water_kills
        << "   (First to " << m_kills_to_win << ")";
    m_hud.setString(ss.str());

    // checking win condition
    if (m_fire_kills >= m_kills_to_win || m_water_kills >= m_kills_to_win)
    {
        RequestStackClear();
        RequestStackPush(StateID::kGameOver);
        return false;
    }

    return true;
}