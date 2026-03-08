// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include "SFML/Graphics/Drawable.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <optional>
#include <vector>
#include <string>

class Bullet
{
public:
    enum class SpellType { Fire, Water };

    Bullet(sf::Vector2f pos, sf::Vector2f dir, int owner_id, SpellType spell);

    void update(sf::Time dt);
    void draw(sf::RenderTarget& target) const;

    bool is_dead() const;
    void kill();

    int owner() const;

    const sf::CircleShape& shape() const;

private:
    sf::CircleShape m_shape;
    sf::Vector2f m_velocity;
    int m_owner_id = 0;
    bool m_dead = false;

    SpellType m_spell = SpellType::Fire;

    std::vector<sf::Texture> m_frames;
    std::optional<sf::Sprite> m_sprite;

    std::size_t m_frame_index = 0;
    sf::Time m_frame_timer = sf::Time::Zero;
    sf::Time m_frame_time = sf::seconds(0.08f);

    sf::Vector2f m_dir = { 1.f, 0.f };

    float m_sprite_scale = 0.10f;
    float m_visual_forward_offset = -180.f;

    void load_frames_from_folder(const std::string& folder);
    void apply_visual_rotation();
};
