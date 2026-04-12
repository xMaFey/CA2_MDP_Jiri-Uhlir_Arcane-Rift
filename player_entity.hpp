// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <optional>
#include "player_input.hpp"

class PlayerEntity
{
public:
    PlayerEntity();

    void set_position(sf::Vector2f p);
    sf::Vector2f position() const;

    void set_color(const sf::Color& c);

    void set_facing_dir(sf::Vector2f dir);


    void update(sf::Time dt, const PlayerInput& input, const std::vector<sf::RectangleShape>& walls);
    void draw(sf::RenderTarget& target) const;

    // networking helpers for animation replication
    int get_net_anim_state() const;
    void apply_network_visual_state(int animState, sf::Vector2f dir, sf::Time dt);

	// Shooting interface
    bool can_shoot() const;
    sf::Vector2f facing_dir() const;
	sf::Vector2f get_projectile_spawn_point(float projectile_radius) const;
    void try_start_shoot_cast(bool shootHeld);
    bool consume_shot_event();

    void respawn(sf::Vector2f p);
    bool is_invulnerable() const { return m_invulnerable; }

    // Networking helpers for respawn blink / spawn protection.
    sf::Time invulnerable_elapsed() const { return m_invulnerable_time; }
    void set_invulnerability_state(bool active, sf::Time elapsed);

    // Wall colision
    static bool circle_rect_intersect(const sf::CircleShape& c, const sf::RectangleShape& r);

    // Call for wizard animations
    void set_animation_root(const std::string& root);

    // Hurtbox for combat
    bool bullet_hits_hurtbox(sf::Vector2f point, float radius) const;

private:
    void resolve_walls(const std::vector<sf::RectangleShape>& walls);

    enum class AnimState
    {
        Idle,
        Run,
        Shoot,
	};

    // animation loading
    void load_animations();
    static std::string dir_to_folder(sf::Vector2f dir);

    // animation playback
    bool set_anim(AnimState st, const std::string& dirFolder);
    void advance_anim(sf::Time dt);
    bool m_anim_looping = true;
    bool m_anim_finished = false;
    

private:
    //render animated sprite
	std::optional<sf::Sprite> m_sprite;

    //root path for animations
    std::string m_anim_root;

    //key: anim state + direction
    std::unordered_map<std::string, std::vector<sf::Texture>> m_anim_textures;

	AnimState m_current_anim_state = AnimState::Idle;
	std::string m_current_anim_dir = "east";

    std::size_t m_frame_index = 0;
    sf::Time m_frame_time = sf::seconds(0.10f);
	sf::Time m_frame_timer = sf::Time::Zero;
	float m_feet_padding = 14.f;

    sf::CircleShape m_body;
    sf::Vector2f m_velocity{ 0.f, 0.f };
    sf::Vector2f m_last_dir{ 1.f, 0.f };

    float m_speed = 200.f;

    // shooting cooldown
    sf::Time m_shoot_cd = sf::seconds(0.2f);
    sf::Time m_shoot_timer = sf::Time::Zero;

    // controls

    bool m_is_shoot_casting = false;
    bool m_shot_event_ready = false;
    bool m_shot_fired_this_cast = false;

    std::size_t m_shoot_release_frame = 4;

    bool m_is_dashing = false;
    sf::Time m_dash_duration = sf::seconds(0.12f);
    sf::Time m_dash_time = sf::Time::Zero;
    sf::Time m_dash_cd = sf::seconds(1.f);
    sf::Time m_dash_cd_timer = sf::Time::Zero;
    float m_dash_speed = 900.f;          // “burst” speed
    sf::Vector2f m_dash_dir{ 1.f, 0.f }; // direction at dash start

    // spawn protection
	bool m_invulnerable = false;
    sf::Time m_invulnerable_time = sf::Time::Zero;
    sf::Time m_invulnerable_duration = sf::seconds(1.5f);

	// hurtbox for combat
    static constexpr float hurtbox_height = 42.f; // vertical lenght
	static constexpr float hurtbox_radius = 14.f; // thickness
};
