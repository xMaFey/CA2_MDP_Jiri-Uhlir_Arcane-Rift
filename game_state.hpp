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

        // If true, the player has requested a team switch.
        // The host applies it only after this player dies.
        bool has_pending_team_change = false;
        GameSettings::Team pending_team = GameSettings::Team::Spectator;
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

private:
    sf::RenderWindow& m_window;

    std::vector<sf::RectangleShape> m_walls;
	std::vector<sf::Vector2f> m_spawn_points;

    std::vector<PlayerSlot> m_players;

    // -1 means the local client has not received its real player id yet.
    int m_local_player_id = -1;

    std::vector<Bullet> m_bullets;

    int m_fire_kills = 0;
    int m_water_kills = 0;
    const int m_kills_to_win = 3;

    std::unique_ptr<HostSession> m_host_session;
    std::unique_ptr<ClientSession> m_client_session;

    std::unordered_map<int, PlayerInput> m_remote_inputs; // newest input per remote player id
    std::optional<WorldStatePacket> m_latest_world_state;

    sf::Text m_hud;

    bool m_pause_open = false;

    sf::RectangleShape m_pause_overlay;
    sf::Text m_pause_title;
    sf::Text m_pause_options;
};
