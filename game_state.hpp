// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "state.hpp"
#include "player_entity.hpp"
#include "player_input.hpp"
#include "bullet.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <vector>
#include "host_session.hpp"
#include "client_session.hpp"
#include <memory>
#include <optional>
#include "network_packets.hpp"
#include <unordered_map>
#include <deque>
#include "container.hpp"
#include "button.hpp"

class GameState : public State
{
public:
    GameState(StateStack& stack, Context context);
    ~GameState() override;

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    struct PlayerSlot
    {
        int id = -1;
        std::string nickname = "Player";
        GameSettings::Team team = GameSettings::Team::Spectator;
        PlayerEntity entity;
        bool dash_prev = false;
        bool connected = false;

        // Latest animation snapshot received from the host.
        // Used by clients to animate remote players every frame.
        int replicated_anim_state = 0;
        sf::Vector2f replicated_dir{ 1.f, 0.f };

        // Client-side helper to detect a fresh respawn from host snapshots.
        bool replicated_invulnerable = false;

        // If true, the player has requested a team switch.
        // The host applies it after a short delay instead of waiting for death.
        bool has_pending_team_change = false;
        GameSettings::Team pending_team = GameSettings::Team::Spectator;
        sf::Time pending_team_change_timer = sf::Time::Zero;
    };

    void build_map();

    // respawn helpers
    sf::Vector2f pick_safe_spawn(const PlayerEntity& enemy) const;
    bool spawn_is_clear(sf::Vector2f p) const;

    PlayerInput build_input_from_keybinds(const PlayerKeybinds& keys, bool& dashPrev);

    PlayerSlot* find_player(int id);
    const PlayerSlot* find_player(int id) const;

    PlayerSlot& ensure_player_slot(int id);

    void queue_team_change(PlayerSlot& player, GameSettings::Team newTeam);
    void apply_team_change_now(PlayerSlot& player, GameSettings::Team newTeam);
    int count_connected_players_on_team(GameSettings::Team team) const;
    bool can_join_team(GameSettings::Team team, int ignorePlayerId = -1) const;
    PlayerSlot* get_local_player_slot();

    // Build a fake "running match" lobby packet so a reconnecting client
    // can leave TeamSelectState and enter the already running match.
    LobbyStatePacket build_running_match_lobby_packet_for_player(int playerId) const;

    void build_pause_gui();
    void OnResize(sf::Vector2u new_size) override;
    void rebuild_pause_layout(sf::Vector2u new_size);
    void update_pause_button_states();

    bool can_switch_local_to_fire() const;
    bool can_switch_local_to_water() const;

    void draw_player_name(sf::RenderTarget& target, const PlayerSlot& player) const;

    void update_camera();
    sf::Vector2f get_camera_target() const;

    void add_local_kill();
    void add_local_death();
    void save_profile_if_needed();
    void finish_match_and_save();

private:
    sf::RenderWindow& m_window;

    // Size of the playable world, larger than the screen.
    sf::Vector2f m_world_size{ 3200.f, 1800.f };

    // Camera used to draw the scrolling game world.
    sf::View m_world_view;

    std::vector<sf::RectangleShape> m_walls;
	std::vector<sf::Vector2f> m_spawn_points;

    std::deque<PlayerSlot> m_players;

    // -1 means the local client has not received its real player id yet.
    int m_local_player_id = -1;

    std::vector<Bullet> m_bullets;
    int m_next_bullet_id = 1; // host assigns unique ids to bullets

    int m_fire_kills = 0;
    int m_water_kills = 0;
    const int m_kills_to_win = 50;

    std::unique_ptr<HostSession> m_host_session;
    std::unique_ptr<ClientSession> m_client_session;

    std::unordered_map<int, PlayerInput> m_remote_inputs; // newest input per remote player id
    std::optional<WorldStatePacket> m_latest_world_state;

    sf::Text m_hud;

    bool m_pause_open = false;

    sf::RectangleShape m_pause_overlay;
    sf::Text m_pause_title;
    sf::Text m_pause_options;

    gui::Container m_pause_gui;

    gui::Button::Ptr m_pause_fire_button;
    gui::Button::Ptr m_pause_water_button;
    gui::Button::Ptr m_pause_spectate_button;
    gui::Button::Ptr m_pause_back_to_menu_button;

    // Persistent local stats for the current match on this machine only.
    int m_local_match_kills = 0;
    int m_local_match_deaths = 0;

    // Prevent saving the same finished match multiple times.
    bool m_match_stats_committed = false;
};
