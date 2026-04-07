// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "player_entity.hpp"
#include <algorithm>
#include <cmath>
#include <SFML/Window/Keyboard.hpp>
#include <iostream>

PlayerEntity::PlayerEntity()
{
    m_body.setRadius(18.f);
    m_body.setOrigin({ 18.f, 18.f });
    m_body.setPosition({ 100.f, 100.f });
	m_body.setFillColor(sf::Color::Transparent);
	m_body.setOutlineThickness(2.f);
	m_body.setOutlineColor(sf::Color(255, 255, 255, 140));
}

static constexpr bool kDrawHurtboxDebug = false;

static std::vector<std::filesystem::path> sorted_pngs(const std::filesystem::path& folder)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(folder)) return files;

    for (auto& e : std::filesystem::directory_iterator(folder))
    {
        if (!e.is_regular_file()) continue;
        auto p = e.path();
        if (p.extension() == ".png")
			files.push_back(p);
    }

    std::sort(files.begin(), files.end());
    return files;
}

void PlayerEntity::set_animation_root(const std::string& root)
{
    m_anim_root = root;

    m_anim_textures.clear();
    load_animations();

    // default pose
    set_anim(AnimState::Idle, "east");

    std::cout << "Anim root:" << m_anim_root << "\n";
}

std::string PlayerEntity::dir_to_folder(sf::Vector2f d)
{
    // if no direction, keep something stable
    const float ax = std::abs(d.x);
    const float ay = std::abs(d.y);

    if (ax < 0.0001f && ay < 0.0001f)
        return "east";

	// 8-directional
    const float a = std::atan2(d.y, d.x);
    float ang = a;
    // split into 8 sectors
    // sector 0 = east, then counter-clockwise
    int sector = static_cast<int>(std::round(ang / (3.14159f / 4.f)));
	// normalize to [0, 7]
	sector = (sector + 8) % 8;

    switch (sector)
    {
    case 0: return "east";
    case 1: return "south_east";
    case 2: return "south";
    case 3: return "south_west";
    case 4: return "west";
    case 5: return "north_west";
    case 6: return "north";
    case 7: return "north_east";
	}
	return "east";
}

void PlayerEntity::load_animations()
{
    const std::vector<std::pair<std::string, std::string>> packs = {
        { "idle", "fight_stance"},
        { "run", "running"},
        { "fireball", "fireball"}
    };

    const std::vector<std::string> dirs = {
        "east", "south_east", "south", "south_west", "west", "north_west", "north", "north_east"
    };

    for (auto& [keyName, folderName] : packs)
    {
        for (auto& dir : dirs)
        {
            const std::filesystem::path dirPath = std::filesystem::path(m_anim_root) / folderName / dir;

			auto frames = sorted_pngs(dirPath);
            if (frames.empty())
                continue;

			std::vector<sf::Texture> textures;
            textures.reserve(frames.size());

            for (auto& f : frames)
            {
                sf::Texture t;
                if (!t.loadFromFile(f.string()))
                {
                    // handle error
                    continue;
                }
				textures.push_back(std::move(t));
			}

            if (!textures.empty())
            {
                m_anim_textures[keyName + "/" + dir] = std::move(textures);
            }
        }
    }

    std::cout << "Loaded anim clips: " << m_anim_textures.size() << "\n";
    for (auto& [k, v] : m_anim_textures)
        std::cout << "  " << k << " frames=" << v.size() << "\n";

}

bool PlayerEntity::set_anim(AnimState st, const std::string& dir)
{
    // build the animation key prefix
    std::string prefix = "idle/";
    if (st == AnimState::Run) prefix = "run/";
    if (st == AnimState::Shoot) prefix = "fireball/";

    const std::string key = prefix + dir;


    auto it = m_anim_textures.find(key);
    if (it == m_anim_textures.end() || it->second.empty())
    {
    std::cout << "Missing anim key: " << key << "\n";
    return false;
    }

    m_current_anim_state = st;
    m_current_anim_dir = dir;

    // looping rules
    m_anim_looping = (st == AnimState::Idle || st == AnimState::Run);
    m_anim_finished = false;

    m_frame_index = 0;
    m_frame_timer = sf::Time::Zero;

    m_sprite.emplace(it->second[m_frame_index]);
	m_sprite->setTexture(it->second[m_frame_index], true);

    // center origin
    auto size = m_sprite->getTexture().getSize();
    m_sprite->setOrigin({
        size.x * 0.5f,
        static_cast<float>(size.y) - m_feet_padding
    });
	m_sprite->setScale({ 2.f, 2.f });

    return true;
}

void PlayerEntity::advance_anim(sf::Time dt)
{
    std::string prefix = "idle/";
    if (m_current_anim_state == AnimState::Run)
        prefix = "run/";
    if (m_current_anim_state == AnimState::Shoot)
        prefix = "fireball/";

    const std::string key = prefix + m_current_anim_dir;

    auto it = m_anim_textures.find(key);
    if (it == m_anim_textures.end() || it->second.empty())
        return;

    if (!m_anim_looping && m_anim_finished)
        return;

    m_frame_timer += dt;
    if (m_frame_timer < m_frame_time)
        return;

    m_frame_timer = sf::Time::Zero;

    const std::size_t frameCount = it->second.size();

    if (m_anim_looping)
    {
        m_frame_index = (m_frame_index + 1) % frameCount;
    }
    else
    {
        // one-shot, advance until the last frame, then mark finished
        if (m_frame_index + 1 < frameCount)
        {
            ++m_frame_index;
        }
        else
        {
            m_anim_finished = true;
        }
    }

    if (m_sprite)
        m_sprite->setTexture(it->second[m_frame_index], true);
}

void PlayerEntity::set_position(sf::Vector2f p) { m_body.setPosition(p); }

void PlayerEntity::set_facing_dir(sf::Vector2f dir)
{
    if (dir.x != 0.f || dir.y != 0.f)
        m_last_dir = dir;
}


sf::Vector2f PlayerEntity::get_projectile_spawn_point(float projectile_radius) const
{
    const sf::Vector2f feet = m_body.getPosition();

    // "hands" height
    const sf::Vector2f hands = feet + sf::Vector2f(0.f, -hurtbox_height * 0.65f);

    sf::Vector2f dir = m_last_dir;
	const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.f) { dir.x /= len; dir.y /= len; }
	else { dir = { 1.f, 0.f }; }

    // push spawn forward so it doesnt start inside your own hurtbox
    const float forward = hurtbox_radius + projectile_radius + 6.f;

    return hands + dir * forward;
}

sf::Vector2f PlayerEntity::position() const { return m_body.getPosition(); }
void PlayerEntity::set_color(const sf::Color& c) { m_body.setOutlineColor(c); }


void PlayerEntity::update(sf::Time dt, const PlayerInput& input, const std::vector<sf::RectangleShape>& walls)
{
    m_shoot_timer += dt;
    try_start_shoot_cast(input.shootHeld);

    if(m_invulnerable)
    {
        m_invulnerable_time += dt;
        if(m_invulnerable_time >= m_invulnerable_duration)
        {
            m_invulnerable = false;
        }
	}

    // dash + melee cd
	m_dash_cd_timer += dt;

    // detect single press for dash
	const bool dash_pressed = input.dashPressed;

    // read movement direction - for facing + normal movement
	sf::Vector2f dir = input.move;

    if (dir.x != 0.f || dir.y != 0.f)
        m_last_dir = dir;

    // dash logic
    if(dash_pressed && !m_is_dashing && m_dash_cd_timer >= m_dash_cd)
    {
        m_is_dashing = true;
        m_dash_time = sf::Time::Zero;
        m_dash_cd_timer = sf::Time::Zero;
        
        // dash in the direction you are facing
		m_dash_dir = (dir.x != 0.f || dir.y != 0.f) ? dir : m_last_dir;
        if (m_dash_dir.x == 0.f && m_dash_dir.y == 0.f)
            m_dash_dir = { 1.f, 0.f };
	}

    // dash overrides normal movement while active (dash is priority)
    if(m_is_dashing)
    {
        m_dash_time += dt;
		m_velocity = m_dash_dir * m_dash_speed;

        if(m_dash_time >= m_dash_duration)
        {
            m_is_dashing = false;
        }
	}
    else
    {
		m_velocity = dir * m_speed;
    }

    // move + collide
    m_body.move(m_velocity * dt.asSeconds());
    resolve_walls(walls);

    // choose anim + direction
	const bool moving = (m_velocity.x != 0.f || m_velocity.y != 0.f);
    const std::string dirFolder = dir_to_folder(m_last_dir);

    // do not override shooting animation with idle/run
    const bool playingShoot = (m_current_anim_state == AnimState::Shoot && !m_anim_finished);

    if (!playingShoot)
    {
        if (moving)
        {
            if (m_current_anim_state != AnimState::Run || m_current_anim_dir != dirFolder)
                set_anim(AnimState::Run, dirFolder);
        }
        else
        {
            if (m_current_anim_state != AnimState::Idle || m_current_anim_dir != dirFolder)
                set_anim(AnimState::Idle, dirFolder);
        }
    }

    advance_anim(dt);

    // fire the projectile at chosen frame - for shooting
    if (m_current_anim_state == AnimState::Shoot && m_is_shoot_casting)
    {
        if (!m_shot_fired_this_cast && m_frame_index >= m_shoot_release_frame)
        {
            // release now
            m_shot_event_ready = true;
            m_shot_fired_this_cast = true;

            // start cd from actual release moment
            m_shoot_timer = sf::Time::Zero;
        }

        // end cast when the animation finishes
        if (m_anim_finished)
        {
            m_is_shoot_casting = false;
        }
    }

    // if shoot finished this frame, return to next correct state frame
    if (m_current_anim_state == AnimState::Shoot && m_anim_finished)
    {
        if (moving)
            set_anim(AnimState::Run, dirFolder);
        else
            set_anim(AnimState::Idle, dirFolder);
    }

    // keep sprite at collission body position
    if (m_sprite)
        m_sprite->setPosition(m_body.getPosition());
}

void PlayerEntity::draw(sf::RenderTarget& target) const
{
    // visible invulnerability - blink effect
    if (m_invulnerable)
    {
        const int tick = static_cast<int>(m_invulnerable_time.asSeconds() * 10.f);
        if (tick % 2 == 1)
			return;
    }

    // debug draw collision body
    target.draw(m_body);

    if (m_sprite) {
        target.draw(*m_sprite);
    }

    // DEBUG: DRAW HURTBOX
    if (kDrawHurtboxDebug) {
        const sf::Vector2f feet = m_body.getPosition();
        const sf::Vector2f torso = feet + sf::Vector2f(0.f, -hurtbox_height);

        const float r = hurtbox_radius;
        const sf::Color col(255, 0, 255, 180);

        sf::CircleShape c(r);
        c.setOrigin({ r, r });
        c.setFillColor(sf::Color::Transparent);
        c.setOutlineThickness(1.f);
        c.setOutlineColor(sf::Color(col));

        c.setPosition(feet);
        target.draw(c);

        c.setPosition(torso);
        target.draw(c);

        sf::Vertex leftLine[] = {
            sf::Vertex{{ feet.x - r, feet.y}, col },
            sf::Vertex{{ torso.x - r, torso.y}, col}
        };

        sf::Vertex rightLine[] = {
            sf::Vertex{{ feet.x + r, feet.y}, col},
            sf::Vertex{{ torso.x + r, torso.y}, col}
        };

        target.draw(leftLine, 2, sf::PrimitiveType::Lines);
		target.draw(rightLine, 2, sf::PrimitiveType::Lines);
    }
}

sf::Vector2f PlayerEntity::facing_dir() const
{
    return m_last_dir;
}

bool PlayerEntity::can_shoot() const
{
    return m_shoot_timer >= m_shoot_cd;
}

void PlayerEntity::try_start_shoot_cast(bool shootHeld)
{
    // only start if - key is held, cd is ready, no already casting
    if (!shootHeld) return;
    if (m_shoot_timer < m_shoot_cd) return;
    if (m_is_shoot_casting) return;

    // start casting animation
    m_is_shoot_casting = true;
    m_shot_fired_this_cast = false;
    m_shot_event_ready = false;

    set_anim(AnimState::Shoot, dir_to_folder(m_last_dir));
}

bool PlayerEntity::consume_shot_event()
{
    if (!m_shot_event_ready) return false;
    m_shot_event_ready = false;
    return true;
}

void PlayerEntity::respawn(sf::Vector2f p)
{
    m_body.setPosition(p);
    m_velocity = { 0.f, 0.f };

    m_invulnerable = true;
    m_invulnerable_time = sf::Time::Zero;

	// reset dash / melee states
    m_is_dashing = false;
	m_dash_time = sf::Time::Zero;
}


bool PlayerEntity::circle_rect_intersect(const sf::CircleShape& c, const sf::RectangleShape& r)
{
    const sf::Vector2f cc = c.getPosition();
    const float radius = c.getRadius();

    const sf::FloatRect rb = r.getGlobalBounds();

    const float closest_x = std::clamp(cc.x, rb.position.x, rb.position.x + rb.size.x);
    const float closest_y = std::clamp(cc.y, rb.position.y, rb.position.y + rb.size.y);

    const float dx = cc.x - closest_x;
    const float dy = cc.y - closest_y;

    return (dx * dx + dy * dy) <= (radius * radius);
}

static float dot(sf::Vector2f a, sf::Vector2f b)
{
    return a.x * b.x + a.y * b.y;
}

static float len2(sf::Vector2f v)
{
    return v.x * v.x + v.y * v.y;
}

// segment vs rect collision
static bool segment_intersect_rect(sf::Vector2f a, sf::Vector2f b, const sf::FloatRect& r)
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
        if (p < 0.f) {
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

	if (!clip(-dx, a.x - r.position.x)) return false;
	if (!clip(dx, (r.position.x + r.size.x) - a.x)) return false;
	if (!clip(-dy, a.y - r.position.y)) return false;
    if (!clip(dy, (r.position.y + r.size.y) - a.y)) return false;

    return true;
}

bool PlayerEntity::bullet_hits_hurtbox(sf::Vector2f point, float radius) const
{
	const sf::Vector2f feet = m_body.getPosition();
    const sf::Vector2f torso = feet + sf::Vector2f(0.f, -hurtbox_height);

    const sf::Vector2f seg = torso - feet;
    const float segLen2 = len2(seg);

    float t = 0.f;
    if (segLen2 > 0.f)
        t = dot(point - feet, seg) / segLen2;
	
    t = std::clamp(t, 0.f, 1.f);
    const sf::Vector2f closest = feet + seg * t;

    const sf::Vector2f d = point - closest;
    const float rr = radius + hurtbox_radius;

    return (d.x * d.x + d.y * d.y) <= (rr * rr);
}

void PlayerEntity::resolve_walls(const std::vector<sf::RectangleShape>& walls)
{
    // Simple resolution: if overlapping, push player out along smallest axis
    for (const auto& w : walls)
    {
        if (!circle_rect_intersect(m_body, w))
            continue;

        const sf::FloatRect rb = w.getGlobalBounds();
        const sf::Vector2f p = m_body.getPosition();
        const float r = m_body.getRadius();

        // compute overlap distances in x/y against rect expanded by circle radius
        const float left = (rb.position.x - r);
        const float right = (rb.position.x + rb.size.x + r);
        const float top = (rb.position.y - r);
        const float bottom = (rb.position.y + rb.size.y + r);

        const float dx_left = std::abs(p.x - left);
        const float dx_right = std::abs(right - p.x);
        const float dy_top = std::abs(p.y - top);
        const float dy_bottom = std::abs(bottom - p.y);

        // push out to nearest edge
        const float minx = std::min(dx_left, dx_right);
        const float miny = std::min(dy_top, dy_bottom);

        sf::Vector2f newp = p;
        if (minx < miny)
        {
            // push horizontally
            newp.x = (dx_left < dx_right) ? left : right;
        }
        else
        {
            // push vertically
            newp.y = (dy_top < dy_bottom) ? top : bottom;
        }
        m_body.setPosition(newp);
    }
}
