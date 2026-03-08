// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "bullet.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <filesystem>
#include <algorithm>
#include <cmath>

Bullet::Bullet(sf::Vector2f pos, sf::Vector2f dir, int owner_id, SpellType spell)
    : m_owner_id(owner_id), m_spell(spell)
{
    // normalize dir
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.f) m_dir = { dir.x / len, dir.y / len };
    else
        m_dir = { 1.f, 0.f };

    m_shape.setRadius(8.f);
    m_shape.setOrigin({ 8.f, 8.f });
    m_shape.setPosition(pos);

    // choose folder based on spell
    const std::string folder = (m_spell == SpellType::Fire)
        ? "Media/Assets/Spells/basic_cast_fire"
        : "Media/Assets/Spells/basic_cast_water";

    load_frames_from_folder(folder);

    if(m_sprite)
        m_sprite->setScale({ m_sprite_scale, m_sprite_scale });

    apply_visual_rotation();

    if (m_sprite)
        m_sprite->setPosition(m_shape.getPosition());


    const float speed = 550.f;
    m_velocity = m_dir * speed;
}

void Bullet::load_frames_from_folder(const std::string& folder)
{
    namespace fs = std::filesystem;

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(folder))
    {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (ext == ".png")
            files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end());

    m_frames.clear();
    m_frames.reserve(files.size());

    for (const auto& p : files)
    {
        sf::Texture t;
        if (t.loadFromFile(p.string()))
            m_frames.push_back(std::move(t));
    }

    if (!m_frames.empty())
    {
        m_sprite.emplace(m_frames[0]);
        m_sprite->setTexture(m_frames[0], true);

        const auto sz = m_sprite->getTexture().getSize();
        m_sprite->setOrigin({ sz.x * 0.5f, sz.y * 0.5f });
    }
}

void Bullet::apply_visual_rotation()
{
    if (!m_sprite) return;
    float deg = std::atan2(m_dir.y, m_dir.x) * 180.f / 3.14f;

    deg += 180.f;

    m_sprite->setRotation(sf::degrees(deg));
}

void Bullet::update(sf::Time dt)
{
    // move collision
    m_shape.move(m_velocity * dt.asSeconds());

    // animate sprite
    if (!m_frames.empty())
    {
        m_frame_timer += dt;
        if (m_frame_timer >= m_frame_time)
        {
            m_frame_timer = sf::Time::Zero;
            m_frame_index = (m_frame_index + 1) % m_frames.size();
            if (m_sprite)
                m_sprite->setTexture(m_frames[m_frame_index], true);
        }
    }

    // set offset to sprite animation
    if (m_sprite)
    {
        const float offset = m_visual_forward_offset * m_sprite_scale;
        const sf::Vector2f visualPos = m_shape.getPosition() + m_dir * offset;
        m_sprite->setPosition(visualPos);
    }
        
}

void Bullet::draw(sf::RenderTarget& target) const
{
    if (m_sprite)
        target.draw(*m_sprite);
    
    // DEBUG HITBOX
    //target.draw(m_shape);
}

bool Bullet::is_dead() const { return m_dead; }
void Bullet::kill() { m_dead = true; }

int Bullet::owner() const { return m_owner_id; }
const sf::CircleShape& Bullet::shape() const { return m_shape; }
