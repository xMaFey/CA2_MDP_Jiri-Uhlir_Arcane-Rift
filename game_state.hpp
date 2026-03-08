// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "state.hpp"
#include "player_entity.hpp"
#include "bullet.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <vector>

class GameState : public State
{
public:
    GameState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    void build_map();
    //void keep_in_bounds(PlayerEntity& p);
    //bool bullet_hits_player(const Bullet& b, const PlayerEntity& p) const;

    // respawn helpers
    sf::Vector2f pick_safe_spawn(const PlayerEntity& enemy) const;
    bool spawn_is_clear(sf::Vector2f p) const;

private:
    sf::RenderWindow& m_window;

    std::vector<sf::RectangleShape> m_walls;
	std::vector<sf::Vector2f> m_spawn_points;

    PlayerEntity m_p1; // arrows + J
    PlayerEntity m_p2; // WASD + `

    std::vector<Bullet> m_bullets;

    int m_kills_p1 = 0;
    int m_kills_p2 = 0;
    const int m_kills_to_win = 5;

    sf::Text m_hud;
};
