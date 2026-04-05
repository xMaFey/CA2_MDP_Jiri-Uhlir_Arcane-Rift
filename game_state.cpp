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
#include <SFML/Network/IpAddress.hpp>
#include "game_settings.hpp"

GameState::GameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_hud(context.fonts->Get(FontID::kMain))
{
	auto& settings = *GetContext().settings;

    GetContext().music->Stop();

    if (GetContext().network)
    {
        auto& network = *GetContext().network;

        if (settings.network_role == GameSettings::NetworkRole::Host)
        {
            m_host_session = std::make_unique<HostSession>(network);

            const bool ok = m_host_session->start(settings.server_port);
            if (ok)
            {
                std::cout << "Host started on port " << settings.server_port << "\n";
            }
            else
            {
                std::cout << "Host failed\n";
            }
        }
        else if (settings.network_role == GameSettings::NetworkRole::Client)
        {
            m_client_session = std::make_unique<ClientSession>(network);

            const auto ip = sf::IpAddress::resolve(settings.server_ip);

            bool ok = false;
            if (ip.has_value())
            {
                ok = m_client_session->connect(*ip, settings.server_port);
            }

            std::cout << (ok ? "Client connected\n" : "Client failed to connect\n");
        }
        else
        {
            network.disconnect();
        }
    }

    build_map();
        
    m_p1.set_color(sf::Color(255, 140, 90));
    m_p1.set_position({ 260.f, 360.f });
    m_p1.set_animation_root("Media/Assets/Characters/wizard_orange/animations/");

    // DEBUG
    if (settings.chosen_team == GameSettings::Team::Fire)
    {
        std::cout << settings.nickname << " joined Fire\n";
    }
    else if (settings.chosen_team == GameSettings::Team::Water)
    {
        std::cout << settings.nickname << " joined Water\n";
    }
    else
    {
        std::cout << settings.nickname << " is Spectator\n";
    }

    m_p2.set_color(sf::Color(70, 200, 255));
    m_p2.set_position({ 1020.f, 360.f });
    m_p2.set_animation_root("Media/Assets/Characters/wizard_blue/animations/");

    m_hud.setCharacterSize(22);
    m_hud.setPosition({ 14.f, 10.f });

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

    if (sf::Keyboard::isKeyPressed(keys.up))    input.move.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.down))  input.move.y += 1.f;
    if (sf::Keyboard::isKeyPressed(keys.left))  input.move.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.right)) input.move.x += 1.f;

    float len = std::sqrt(input.move.x * input.move.x + input.move.y * input.move.y);
    if (len > 0.f)
    {
        input.move.x /= len;
        input.move.y /= len;
    }

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

static float dist2(sf::Vector2f a, sf::Vector2f b)
{
    sf::Vector2f d = a - b;
	return d.x * d.x + d.y * d.y;
}

static bool rect_circle_hit(const sf::FloatRect& r, sf::Vector2f c, float cr)
{
	const float closest_x = std::clamp(c.x, r.position.x, r.position.x + r.size.x);
	const float closest_y = std::clamp(c.y, r.position.y, r.position.y + r.size.y);

	const float dx = c.x - closest_x;
	const float dy = c.y - closest_y;

	return (dx * dx + dy * dy) <= (cr * cr);
}
static bool segment_intersects_rect(sf::Vector2f a, sf::Vector2f b, const sf::FloatRect& r)
{
    // Liang-Barsky algorithm
    const float dx = b.x - a.x;
	const float dy = b.y - a.y;

	float t0 = 0.f, t1 = 1.f;

    auto clip = [&](float p, float q) -> bool
        {
            if (p == 0.f)
                return q >= 0.f;
            const float t = q / p;
            if (p < 0.f)
            {
                if (t > t1) return false;
                if (t > t0) t0 = t;
            }
            else
            {
                if (t < t0) return false;
                if (t < t1) t1 = t;
            }
            return true;
        };

    if (!clip( -dx, a.x - r.position.x)) return false;
	if (!clip(dx, (r.position.x) + r.size.x - a.x)) return false;
	if (!clip(-dy, a.y - r.position.y)) return false;
    if (!clip(dy, (r.position.y + r.size.y) - a.y)) return false;

    return true;
}

static bool segment_hits_any_wall(sf::Vector2f a, sf::Vector2f b, const std::vector<sf::RectangleShape>& walls)
{
    for (const auto& w : walls)
    {
        if (segment_intersects_rect(a, b, w.getGlobalBounds()))
            return true;
    }
	return false;
}

//static bool circle_circle_hit(const sf::CircleShape& a, const sf::CircleShape& b)
//{
//    const sf::Vector2f pa = a.getPosition();
//    const sf::Vector2f pb = b.getPosition();
//    const float ra = a.getRadius();
//    const float rb = b.getRadius();
//
//    const float dx = pa.x - pb.x;
//    const float dy = pa.y - pb.y;
//    const float rr = ra + rb;
//
//    return (dx * dx + dy * dy) <= (rr * rr);
//}

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

    // Borders
    make_wall(0.f, 0.f, W, 20.f);
    make_wall(0.f, H - 20.f, W, 20.f);
    make_wall(0.f, 0.f, 20.f, H);
    make_wall(W - 20.f, 0.f, 20.f, H);

    // Maze interior walls
    make_wall(W * 0.20f, H * 0.18f, 24.f, H * 0.52f);
    make_wall(W * 0.35f, H * 0.30f, W * 0.30f, 24.f);
    make_wall(W * 0.55f, H * 0.18f, 24.f, H * 0.55f);
    make_wall(W * 0.25f, H * 0.72f, W * 0.35f, 24.f);
    make_wall(W * 0.72f, H * 0.40f, 24.f, H * 0.40f);
}

bool GameState::spawn_is_clear(sf::Vector2f p) const
{
    // probe circle
    sf::CircleShape probe;
	probe.setRadius(18.f);
	probe.setOrigin({ 18.f, 18.f });
	probe.setPosition(p);

    for(const auto& w : m_walls)
    {
        if (PlayerEntity::circle_rect_intersect(probe, w))
            return false;
	}
    return true;
}

sf::Vector2f GameState::pick_safe_spawn(const PlayerEntity& enemy) const
{
    // pick spawn far from enemy and not inside walls
    const float min_dist = 200.f;
	const float min_dist_sq = min_dist * min_dist;

    // copy + shuffle spawn points
	std::vector<sf::Vector2f> candidates = m_spawn_points;
	static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(candidates.begin(), candidates.end(), rng);

	// fist pass: far enough from enemy + clear
    for (const auto& sp : candidates)
    {
        if(!spawn_is_clear(sp)) continue;
        if (dist2(sp, enemy.position()) >= min_dist_sq)
            return sp;
	}

    // choose the clear spawn that is farthest from enemy
    sf::Vector2f best = candidates.front();
    float best_d2 = -1.f;

    for(const auto& sp : candidates)
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
    // Background
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(m_window.getSize()));
    bg.setFillColor(sf::Color(18, 18, 28));
    target.draw(bg);

    for (auto& w : m_walls) target.draw(w);
    for (auto& b : m_bullets) b.draw(target);

    // depth sort
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

    target.draw(m_hud);
}

bool GameState::Update(sf::Time dt)
{
    // Update players with wall collision
    const auto& settings = *GetContext().settings;

    PlayerInput p1Input{};
    PlayerInput p2Input{};

    // build local input based on selected team
    if (settings.chosen_team == GameSettings::Team::Fire)
    {
        p1Input = build_input_from_keybinds(settings.local_keys, m_p1_dash_prev);
    }
    else if (settings.chosen_team == GameSettings::Team::Water)
    {
        p2Input = build_input_from_keybinds(settings.local_keys, m_p2_dash_prev);
    }

    // networking step 1:
    // host receives remote input
    // client sends local input
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            const auto remote = m_host_session->poll_remote_input();
            if (remote.has_value())
            {
                m_remote_input = *remote;
            }
        }

        // apply latest remote input to the opposite player
        if (settings.chosen_team == GameSettings::Team::Fire)
        {
            if (m_remote_input.has_value())
                p2Input = *m_remote_input;
        }
        else if (settings.chosen_team == GameSettings::Team::Water)
        {
            if (m_remote_input.has_value())
                p1Input = *m_remote_input;
        }
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_client_session)
        {
            if (settings.chosen_team == GameSettings::Team::Fire)
            {
                m_client_session->send_local_input(p1Input);
            }
            else if (settings.chosen_team == GameSettings::Team::Water)
            {
                m_client_session->send_local_input(p2Input);
            }
        }
    }

    m_p1.update(dt, p1Input, m_walls);
    m_p2.update(dt, p2Input, m_walls);

    if (settings.network_role != GameSettings::NetworkRole::Client)
    {
        // Shooting - spawn bullets only when shoot animation reaches release frame
        if (m_p1.consume_shot_event())
        {
            GetContext().sounds->Play(SoundID::kFireSpell);
            m_bullets.emplace_back(m_p1.get_projectile_spawn_point(6.f), m_p1.facing_dir(), 1, Bullet::SpellType::Fire);
        }
        if (m_p2.consume_shot_event())
        {
            GetContext().sounds->Play(SoundID::kWaterSpell);
            m_bullets.emplace_back(m_p2.get_projectile_spawn_point(6.f), m_p2.facing_dir(), 2, Bullet::SpellType::Water);
        }

        // Update bullets
        for (auto& b : m_bullets) b.update(dt);

        // Bullet vs walls -> kill bullet
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

        // Bullet vs players - if invulnerable = ignore
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            const sf::Vector2f bp = b.shape().getPosition();
            const float br = b.shape().getRadius();

            if (b.owner() == 1 && !m_p2.is_invulnerable() && m_p2.bullet_hits_hurtbox(bp, br))
            {
                b.kill();
                GetContext().sounds->Play(SoundID::kFireHit);
                ++m_fire_kills;
                m_p2.respawn(pick_safe_spawn(m_p1));
            }
            else if (b.owner() == 2 && !m_p1.is_invulnerable() && m_p1.bullet_hits_hurtbox(bp, br))
            {
                b.kill();
                GetContext().sounds->Play(SoundID::kWaterHit);
                ++m_water_kills;
                m_p1.respawn(pick_safe_spawn(m_p2));
            }
        }

        // Remove dead bullets
        m_bullets.erase(
            std::remove_if(m_bullets.begin(), m_bullets.end(), [](const Bullet& b) { return b.is_dead(); }),
            m_bullets.end()
        );
    }

    // host sends current world state to client
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            WorldStatePacket state;
            state.p1_pos = m_p1.position();
            state.p2_pos = m_p2.position();
            state.fire_kills = m_fire_kills;
            state.water_kills = m_water_kills;

            // add bullets
            for (const auto& b : m_bullets)
            {
                if (b.is_dead()) continue;

                BulletState bs;
                bs.pos = b.shape().getPosition();
                bs.dir = b.direction();
                bs.owner = b.owner();

                // spell type
                bs.spell = (b.owner() == 1) ? 0 : 1;

                state.bullets.push_back(bs);
            }

            m_host_session->send_world_state(state);
        }
    }

    // client receives world state from host and applies it
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
                m_p1.set_position(m_latest_world_state->p1_pos);
                m_p2.set_position(m_latest_world_state->p2_pos);

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
    
    // HUD
    std::ostringstream ss;
    ss << "Fire: " << m_fire_kills << "   |   Water: " << m_water_kills << "   (First to " << m_kills_to_win << ")";
    m_hud.setString(ss.str());
            
    // Win condition
    if (m_fire_kills >= m_kills_to_win || m_water_kills >= m_kills_to_win)
    {
		RequestStackClear();
        RequestStackPush(StateID::kGameOver);
        return false;
    }

    return true;
}
