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

class PlayerEntity
{
public:
    PlayerEntity();

    void set_position(sf::Vector2f p);
    sf::Vector2f position() const;

    void set_color(const sf::Color& c);

    // Controls
    void set_controls_arrows();
    void set_controls_wasd();

    void update(sf::Time dt, const std::vector<sf::RectangleShape>& walls);
    void draw(sf::RenderTarget& target) const;

	// Shooting interface
    bool can_shoot() const;
    sf::Vector2f facing_dir() const;
	sf::Vector2f get_projectile_spawn_point(float projectile_radius) const;
    void try_start_shoot_cast();
    bool consume_shot_event();

    void respawn(sf::Vector2f p);
    bool is_invulnerable() const { return m_invulnerable; }

    // Dash / Melee interface
	bool is_melee_active() const { return m_melee_active; }
	sf::FloatRect get_melee_hitbox_world() const;

    // Wall colision
    static bool circle_rect_intersect(const sf::CircleShape& c, const sf::RectangleShape& r);

    // Call for wizard animations
    void set_animation_root(const std::string& root);

    // Hurtbox for combat
    bool bullet_hits_hurtbox(sf::Vector2f point, float radius) const;
	bool rect_hits_hurtbox(const sf::FloatRect& rect) const;

private:
    void handle_input(sf::Vector2f& dir) const;
    void resolve_walls(const std::vector<sf::RectangleShape>& walls);

    enum class AnimState
    {
        Idle,
        Run,
        Shoot,
        Melee,
        Dash
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
    // movement
    sf::Keyboard::Scancode m_up{};
    sf::Keyboard::Scancode m_down{};
    sf::Keyboard::Scancode m_left{};
    sf::Keyboard::Scancode m_right{};

    // shooting
    sf::Keyboard::Scancode m_shoot{};

    bool m_is_shoot_casting = false;
    bool m_shot_event_ready = false;
    bool m_shot_fired_this_cast = false;

    std::size_t m_shoot_release_frame = 4;

    // melee attack
    sf::Keyboard::Scancode m_melee{};

    bool m_melee_active = false;
    bool m_melee_pressed_prev = false;

    sf::Time m_melee_active_window = sf::seconds(0.10f);
    sf::Time m_melee_active_time = sf::Time::Zero;
    sf::Time m_melee_cd = sf::seconds(0.5f);
    sf::Time m_melee_cd_timer = sf::Time::Zero;

    // dash
    sf::Keyboard::Scancode m_dash{};

    bool m_dash_pressed_prev = false;
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
