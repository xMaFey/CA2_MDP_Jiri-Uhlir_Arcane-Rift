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

class GameState : public State
{
public:
    GameState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    void build_map();

    // respawn helpers
    sf::Vector2f pick_safe_spawn(const PlayerEntity& enemy) const;
    bool spawn_is_clear(sf::Vector2f p) const;

    PlayerInput build_input_from_keybinds(const PlayerKeybinds& keys, bool& dashPrev);

private:
    sf::RenderWindow& m_window;

    std::vector<sf::RectangleShape> m_walls;
	std::vector<sf::Vector2f> m_spawn_points;

    PlayerEntity m_p1; // host-side player slot
    PlayerEntity m_p2; // client-side player slot

    GameSettings::Team m_p1_team = GameSettings::Team::Spectator; // host side
    GameSettings::Team m_p2_team = GameSettings::Team::Spectator; // client side

    bool m_p1_dash_prev = false;
    bool m_p2_dash_prev = false;

    std::vector<Bullet> m_bullets;

    int m_fire_kills = 0;
    int m_water_kills = 0;
    const int m_kills_to_win = 3;

    std::unique_ptr<HostSession> m_host_session;
    std::unique_ptr<ClientSession> m_client_session;

    std::optional<PlayerInput> m_remote_input;
    std::optional<WorldStatePacket> m_latest_world_state;

    sf::Text m_hud;
};
