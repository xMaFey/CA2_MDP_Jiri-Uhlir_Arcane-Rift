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

    Bullet(int bullet_id, sf::Vector2f pos, sf::Vector2f dir, int owner_id, SpellType spell);

    void update(sf::Time dt);
    void draw(sf::RenderTarget& target) const;

    bool is_dead() const;
    void kill();

    int owner() const;

    const sf::CircleShape& shape() const;

    sf::Vector2f direction() const { return m_dir; }

    int bullet_id() const { return m_bullet_id; }

    // Host/client snapshot sync helpers.
    // Client bullets can be corrected to the latest host snapshot
    // instead of being destroyed and recreated every packet.
    void set_position(sf::Vector2f pos);
    void set_direction(sf::Vector2f dir);

private:
    sf::CircleShape m_shape;
    sf::Vector2f m_velocity;
    int m_owner_id = 0;
    int m_bullet_id = -1;
    bool m_dead = false;

    SpellType m_spell = SpellType::Fire;

    const std::vector<sf::Texture>* m_frames = nullptr;
    std::optional<sf::Sprite> m_sprite;

    std::size_t m_frame_index = 0;
    sf::Time m_frame_timer = sf::Time::Zero;
    sf::Time m_frame_time = sf::seconds(0.08f);

    sf::Vector2f m_dir = { 1.f, 0.f };

    float m_sprite_scale = 0.10f;
    float m_visual_forward_offset = -180.f;

    static std::vector<sf::Texture> s_fire_frames;
    static std::vector<sf::Texture> s_water_frames;
    static bool s_frames_loaded;

    static void ensure_shared_frames_loaded();
    static void load_frames_from_folder(const std::string& folder, std::vector<sf::Texture>& out_frames);

    void apply_visual_rotation();
};
