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

GameState::GameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_hud(context.fonts->Get(FontID::kMain))
{
    GetContext().music->Stop();

    build_map();

    m_p1.set_controls_wasd();
    m_p1.set_color(sf::Color(255, 140, 90));
    m_p1.set_position({ 260.f, 360.f });
    m_p1.set_animation_root("Media/Assets/Characters/wizard_orange/animations/");


    m_p2.set_controls_arrows();
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

static int g_winner = 1;

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
    m_p1.update(dt, m_walls);
    m_p2.update(dt, m_walls);

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
            ++m_kills_p1;
            m_p2.respawn(pick_safe_spawn(m_p1));
        }
        else if (b.owner() == 2 && !m_p1.is_invulnerable() && m_p1.bullet_hits_hurtbox(bp, br))
        {
            b.kill();
            GetContext().sounds->Play(SoundID::kWaterHit);
            ++m_kills_p2;
            m_p1.respawn(pick_safe_spawn(m_p2));
        }
    }

    // P1 melee hits P2
    if (m_p1.is_melee_active() && !m_p2.is_invulnerable())
    {
        if (m_p2.rect_hits_hurtbox(m_p1.get_melee_hitbox_world()))
        {
			if (!segment_hits_any_wall(m_p1.position(), m_p2.position(), m_walls))
            {
                ++m_kills_p1;
                m_p2.respawn(pick_safe_spawn(m_p1));
            }
        }
    }

    // P2 melee hits P1
    if (m_p2.is_melee_active() && !m_p1.is_invulnerable())
    {
        if (m_p1.rect_hits_hurtbox(m_p2.get_melee_hitbox_world()))
        {
            if (!segment_hits_any_wall(m_p2.position(), m_p1.position(), m_walls))
            {
                ++m_kills_p2;
                m_p1.respawn(pick_safe_spawn(m_p2));
            }
        }
    }
    
    // Remove dead bullets
    m_bullets.erase(
        std::remove_if(m_bullets.begin(), m_bullets.end(), [](const Bullet& b) { return b.is_dead(); }),
        m_bullets.end()
    );
    
    // HUD
    std::ostringstream ss;
    ss << "P1 Kills: " << m_kills_p1 << "   |   P2 Kills: " << m_kills_p2 << "   (First to " << m_kills_to_win << ")";
    m_hud.setString(ss.str());
            
    // Win condition
    if (m_kills_p1 >= m_kills_to_win || m_kills_p2 >= m_kills_to_win)
    {
        // store winner in Context Player
        if (GetContext().player)
        {
            if (m_kills_p1 >= m_kills_to_win)
                GetContext().player->SetWinner(Player::Winner::kP1);
            else
                GetContext().player->SetWinner(Player::Winner::kP2);
        }

        // go to win screen
		RequestStackClear();
        RequestStackPush(StateID::kGameOver);
        return false;
    }

    return true;
}

bool GameState::HandleEvent(const sf::Event & event)
{
    if (const auto* keypress = event.getIf<sf::Event::KeyPressed>())
    {
        if (keypress->scancode == sf::Keyboard::Scancode::Escape)
        {
            RequestStackPush(StateID::kPause);
        }
    }
    return true;
}
