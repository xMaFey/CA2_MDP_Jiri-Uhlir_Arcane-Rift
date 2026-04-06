// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "game_state.hpp"
#include "fontID.hpp"
#include "stateid.hpp"
#include <algorithm>
#include <sstream>
#include <random>
#include <iostream>
#include <cmath>
#include <SFML/Network/IpAddress.hpp>
#include "game_settings.hpp"

namespace
{
    // checking if the player is actually in one of the two fighting teams
    bool is_combat_team(GameSettings::Team t)
    {
        return t == GameSettings::Team::Fire || t == GameSettings::Team::Water;
    }

    // applying team visuals to the player slot
    void apply_team_visuals(PlayerEntity& player, GameSettings::Team team)
    {
        if (team == GameSettings::Team::Fire)
        {
            player.set_color(sf::Color(255, 140, 90));
            player.set_animation_root("Media/Assets/Characters/wizard_orange/animations/");
        }
        else if (team == GameSettings::Team::Water)
        {
            player.set_color(sf::Color(70, 200, 255));
            player.set_animation_root("Media/Assets/Characters/wizard_blue/animations/");
        }
    }

    // converting network team id into local enum
    GameSettings::Team decode_team(int t)
    {
        if (t == static_cast<int>(NetTeam::Fire)) return GameSettings::Team::Fire;
        if (t == static_cast<int>(NetTeam::Water)) return GameSettings::Team::Water;
        return GameSettings::Team::Spectator;
    }

    // converting local enum into network team id
    int encode_team(GameSettings::Team t)
    {
        if (t == GameSettings::Team::Fire) return static_cast<int>(NetTeam::Fire);
        if (t == GameSettings::Team::Water) return static_cast<int>(NetTeam::Water);
        return static_cast<int>(NetTeam::Spectator);
    }

    // converting team enum to text for HUD
    std::string team_to_string(GameSettings::Team t)
    {
        if (t == GameSettings::Team::Fire) return "Fire";
        if (t == GameSettings::Team::Water) return "Water";
        return "Spectator";
    }

    // squared distance helper for respawn logic
    float dist2(sf::Vector2f a, sf::Vector2f b)
    {
        sf::Vector2f d = a - b;
        return d.x * d.x + d.y * d.y;
    }

    // default spawn positions for first few player ids
    sf::Vector2f spawn_for_player_id(int id)
    {
        static const std::vector<sf::Vector2f> spawns =
        {
            {260.f, 360.f},
            {1020.f, 360.f},
            {150.f, 140.f},
            {1130.f, 140.f},
            {150.f, 620.f},
            {1130.f, 620.f},
            {640.f, 360.f}
        };

        if (id >= 0 && id < static_cast<int>(spawns.size()))
            return spawns[id];

        return spawns.back();
    }
}

GameState::GameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_window(*context.window)
    , m_hud(context.fonts->Get(FontID::kMain))
{
    auto& settings = *GetContext().settings;

    GetContext().music->Stop();

    // starting networking
    if (GetContext().network)
    {
        auto& network = *GetContext().network;

        if (settings.network_role == GameSettings::NetworkRole::Host)
        {
            m_host_session = std::make_unique<HostSession>(network);

            const bool ok = m_host_session->start(settings.server_port);
            if (ok)
                std::cout << "Host started on port " << settings.server_port << "\n";
            else
                std::cout << "Host failed\n";
        }
        else if (settings.network_role == GameSettings::NetworkRole::Client)
        {
            m_client_session = std::make_unique<ClientSession>(network);

            const auto ip = sf::IpAddress::resolve(settings.server_ip);

            bool ok = false;
            if (ip.has_value())
                ok = m_client_session->connect(*ip, settings.server_port);

            std::cout << (ok ? "Client connected\n" : "Client failed to connect\n");

            // sending chosen nickname and team to host once after connect
            if (ok && m_client_session)
            {
                JoinInfoPacket joinInfo;
                joinInfo.nickname = settings.nickname;
                joinInfo.team = encode_team(settings.chosen_team);

                m_client_session->send_join_info(joinInfo);
            }
        }
        else
        {
            network.disconnect();
        }
    }

    build_map();

    // building player slot list
    m_players.clear();

    PlayerSlot hostSlot;
    hostSlot.id = 0;
    hostSlot.nickname = (settings.network_role == GameSettings::NetworkRole::Client) ? "Host" : settings.nickname;
    hostSlot.team = GameSettings::Team::Spectator;
    hostSlot.connected = true;
    hostSlot.entity.set_position(spawn_for_player_id(0));

    PlayerSlot clientSlot;
    clientSlot.id = 1;
    clientSlot.nickname = "Player";
    clientSlot.team = GameSettings::Team::Spectator;
    clientSlot.connected = (settings.network_role == GameSettings::NetworkRole::None);
    clientSlot.entity.set_position(spawn_for_player_id(1));

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        m_local_player_id = 0;
        hostSlot.team = settings.chosen_team;
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        m_local_player_id = 1;
        clientSlot.nickname = settings.nickname;
        clientSlot.team = settings.chosen_team;
        clientSlot.connected = true;
    }
    else
    {
        // offline fallback
        m_local_player_id = 0;
        hostSlot.team = GameSettings::Team::Fire;
        clientSlot.team = GameSettings::Team::Water;
    }

    if (is_combat_team(hostSlot.team))
        apply_team_visuals(hostSlot.entity, hostSlot.team);

    if (is_combat_team(clientSlot.team))
        apply_team_visuals(clientSlot.entity, clientSlot.team);

    m_players.push_back(std::move(hostSlot));
    m_players.push_back(std::move(clientSlot));


    m_hud.setCharacterSize(20);
    m_hud.setPosition({ 14.f, 10.f });

    // spawn points used when a player dies
    m_spawn_points =
    {
        {150.f, 140.f},
        {150.f, 620.f},
        {1130.f, 140.f},
        {1130.f, 620.f},
        {640.f, 360.f}
    };
}

GameState::PlayerSlot* GameState::find_player(int id)
{
    for (auto& p : m_players)
    {
        if (p.id == id)
            return &p;
    }

    return nullptr;
}

const GameState::PlayerSlot* GameState::find_player(int id) const
{
    for (const auto& p : m_players)
    {
        if (p.id == id)
            return &p;
    }

    return nullptr;
}

PlayerInput GameState::build_input_from_keybinds(const PlayerKeybinds& keys, bool& dashPrev)
{
    PlayerInput input;

    // movement keys
    if (sf::Keyboard::isKeyPressed(keys.up))    input.move.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.down))  input.move.y += 1.f;
    if (sf::Keyboard::isKeyPressed(keys.left))  input.move.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(keys.right)) input.move.x += 1.f;

    // preventing faster diagonal movement
    float len = std::sqrt(input.move.x * input.move.x + input.move.y * input.move.y);
    if (len > 0.f)
    {
        input.move.x /= len;
        input.move.y /= len;
    }

    // combat keys
    input.shootHeld = sf::Keyboard::isKeyPressed(keys.shoot);

    bool dashNow = sf::Keyboard::isKeyPressed(keys.dash);
    input.dashPressed = dashNow && !dashPrev;
    dashPrev = dashNow;

    return input;
}

bool GameState::HandleEvent(const sf::Event& event)
{
    return true;
}

void GameState::build_map()
{
    m_walls.clear();

    auto make_wall = [&](float x, float y, float w, float h)
        {
            sf::RectangleShape r;
            r.setSize({ w, h });
            r.setPosition({ x, y });
            r.setFillColor(sf::Color(40, 40, 55));
            m_walls.push_back(r);
        };

    const float W = static_cast<float>(m_window.getSize().x);
    const float H = static_cast<float>(m_window.getSize().y);

    // border walls
    make_wall(0.f, 0.f, W, 20.f);
    make_wall(0.f, H - 20.f, W, 20.f);
    make_wall(0.f, 0.f, 20.f, H);
    make_wall(W - 20.f, 0.f, 20.f, H);

    // inside walls
    make_wall(W * 0.20f, H * 0.18f, 24.f, H * 0.52f);
    make_wall(W * 0.35f, H * 0.30f, W * 0.30f, 24.f);
    make_wall(W * 0.55f, H * 0.18f, 24.f, H * 0.55f);
    make_wall(W * 0.25f, H * 0.72f, W * 0.35f, 24.f);
    make_wall(W * 0.72f, H * 0.40f, 24.f, H * 0.40f);
}

bool GameState::spawn_is_clear(sf::Vector2f p) const
{
    // checking that the respawn point is not inside a wall
    sf::CircleShape probe;
    probe.setRadius(18.f);
    probe.setOrigin({ 18.f, 18.f });
    probe.setPosition(p);

    for (const auto& w : m_walls)
    {
        if (PlayerEntity::circle_rect_intersect(probe, w))
            return false;
    }

    return true;
}

sf::Vector2f GameState::pick_safe_spawn(const PlayerEntity& enemy) const
{
    // choosing a spawn away from the enemy
    const float min_dist = 200.f;
    const float min_dist_sq = min_dist * min_dist;

    std::vector<sf::Vector2f> candidates = m_spawn_points;
    static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(candidates.begin(), candidates.end(), rng);

    // first pass: clear spawn with enough distance
    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;
        if (dist2(sp, enemy.position()) >= min_dist_sq)
            return sp;
    }

    // second pass: farthest clear spawn
    sf::Vector2f best = candidates.front();
    float best_d2 = -1.f;

    for (const auto& sp : candidates)
    {
        if (!spawn_is_clear(sp)) continue;

        const float d2 = dist2(sp, enemy.position());
        if (d2 > best_d2)
        {
            best_d2 = d2;
            best = sp;
        }
    }

    return best;
}

void GameState::Draw(sf::RenderTarget& target)
{
    // background
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(m_window.getSize()));
    bg.setFillColor(sf::Color(18, 18, 28));
    target.draw(bg);

    // map and bullets
    for (auto& w : m_walls) target.draw(w);
    for (auto& b : m_bullets) b.draw(target);

    // draw connected combat players sorted by Y
    std::vector<const PlayerSlot*> drawPlayers;
    for (const auto& p : m_players)
    {
        if (p.connected && is_combat_team(p.team))
            drawPlayers.push_back(&p);
    }

    std::sort(drawPlayers.begin(), drawPlayers.end(),
        [](const PlayerSlot* a, const PlayerSlot* b)
        {
            return a->entity.position().y < b->entity.position().y;
        });

    for (const auto* p : drawPlayers)
        p->entity.draw(target);

    target.draw(m_hud);
}

bool GameState::Update(sf::Time dt)
{
    const auto& settings = *GetContext().settings;

    PlayerSlot* hostPlayer = find_player(0);
    PlayerSlot* clientPlayer = find_player(1);
    PlayerSlot* localPlayer = find_player(m_local_player_id);

    PlayerInput hostInput{};
    PlayerInput clientInput{};

    // local machine builds input only for the player it owns
    if (localPlayer && localPlayer->connected && is_combat_team(localPlayer->team))
    {
        PlayerInput localBuilt = build_input_from_keybinds(settings.local_keys, localPlayer->dash_prev);

        if (localPlayer->id == 0)
            hostInput = localBuilt;
        else if (localPlayer->id == 1)
            clientInput = localBuilt;
    }

    // host receives join info first, then newest remote input
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session)
        {
            if (clientPlayer && !clientPlayer->connected)
            {
                while (true)
                {
                    const auto joinInfo = m_host_session->poll_join_info();
                    if (!joinInfo.has_value())
                        break;

                    clientPlayer->connected = true;
                    clientPlayer->nickname = joinInfo->nickname;
                    clientPlayer->team = decode_team(joinInfo->team);
                    clientPlayer->entity.set_position(spawn_for_player_id(clientPlayer->id));

                    if (is_combat_team(clientPlayer->team))
                        apply_team_visuals(clientPlayer->entity, clientPlayer->team);

                    std::cout << "Client joined: " << clientPlayer->nickname << "\n";
                }
            }

            while (true)
            {
                const auto remote = m_host_session->poll_remote_input();
                if (!remote.has_value())
                    break;

                m_remote_input = *remote;
            }
        }

        if (m_remote_input.has_value() && clientPlayer && clientPlayer->connected && is_combat_team(clientPlayer->team))
            clientInput = *m_remote_input;
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_client_session && localPlayer && localPlayer->id == 1 && localPlayer->connected && is_combat_team(localPlayer->team))
            m_client_session->send_local_input(clientInput);
    }

    // offline fallback
    if (settings.network_role == GameSettings::NetworkRole::None)
    {
        if (hostPlayer && hostPlayer->connected && is_combat_team(hostPlayer->team))
            hostInput = build_input_from_keybinds(settings.local_keys, hostPlayer->dash_prev);
    }

    // update all connected combat players
    for (auto& p : m_players)
    {
        if (!p.connected || !is_combat_team(p.team))
            continue;

        PlayerInput input{};
        if (p.id == 0) input = hostInput;
        else if (p.id == 1) input = clientInput;

        p.entity.update(dt, input, m_walls);
    }

    // host/offline simulates bullets and combat
    if (settings.network_role != GameSettings::NetworkRole::Client)
    {
        // spawn bullets from all active players
        for (auto& p : m_players)
        {
            if (!p.connected || !is_combat_team(p.team))
                continue;

            if (!p.entity.consume_shot_event())
                continue;

            if (p.team == GameSettings::Team::Fire)
            {
                GetContext().sounds->Play(SoundID::kFireSpell);
                m_bullets.emplace_back(
                    p.entity.get_projectile_spawn_point(6.f),
                    p.entity.facing_dir(),
                    p.id,
                    Bullet::SpellType::Fire
                );
            }
            else if (p.team == GameSettings::Team::Water)
            {
                GetContext().sounds->Play(SoundID::kWaterSpell);
                m_bullets.emplace_back(
                    p.entity.get_projectile_spawn_point(6.f),
                    p.entity.facing_dir(),
                    p.id,
                    Bullet::SpellType::Water
                );
            }
        }

        // move bullets
        for (auto& b : m_bullets)
            b.update(dt);

        // bullet vs wall collision
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            const auto bb = b.shape().getGlobalBounds();

            for (const auto& w : m_walls)
            {
                if (bb.findIntersection(w.getGlobalBounds()).has_value())
                {
                    b.kill();
                    break;
                }
            }
        }

        // bullet vs players
        for (auto& b : m_bullets)
        {
            if (b.is_dead()) continue;

            PlayerSlot* shooter = find_player(b.owner());
            if (!shooter || !shooter->connected)
                continue;

            const sf::Vector2f bp = b.shape().getPosition();
            const float br = b.shape().getRadius();

            for (auto& targetPlayer : m_players)
            {
                if (!targetPlayer.connected || !is_combat_team(targetPlayer.team))
                    continue;

                if (targetPlayer.id == b.owner())
                    continue;

                if (targetPlayer.entity.is_invulnerable())
                    continue;

                const bool sameTeam = (shooter->team == targetPlayer.team);

                // preventing team kills
                if (!sameTeam && targetPlayer.entity.bullet_hits_hurtbox(bp, br))
                {
                    b.kill();

                    if (shooter->team == GameSettings::Team::Fire)
                    {
                        GetContext().sounds->Play(SoundID::kFireHit);
                        ++m_fire_kills;
                    }
                    else if (shooter->team == GameSettings::Team::Water)
                    {
                        GetContext().sounds->Play(SoundID::kWaterHit);
                        ++m_water_kills;
                    }

                    targetPlayer.entity.respawn(pick_safe_spawn(shooter->entity));
                    break;
                }
            }
        }

        // remove dead bullets
        m_bullets.erase(
            std::remove_if(
                m_bullets.begin(),
                m_bullets.end(),
                [](const Bullet& b) { return b.is_dead(); }
            ),
            m_bullets.end()
        );
    }

    // host still sends only first 2 slots for now
    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (m_host_session && hostPlayer && clientPlayer)
        {
            WorldStatePacket state;
            state.p1_pos = hostPlayer->entity.position();
            state.p2_pos = clientPlayer->entity.position();
            state.p1_dir = hostPlayer->entity.facing_dir();
            state.p2_dir = clientPlayer->entity.facing_dir();
            state.p1_team = encode_team(hostPlayer->team);
            state.p2_team = encode_team(clientPlayer->team);
            state.fire_kills = m_fire_kills;
            state.water_kills = m_water_kills;

            for (const auto& b : m_bullets)
            {
                if (b.is_dead()) continue;

                BulletState bs;
                bs.pos = b.shape().getPosition();
                bs.dir = b.direction();
                bs.owner = b.owner();

                PlayerSlot* ownerPlayer = find_player(b.owner());
                if (ownerPlayer && ownerPlayer->team == GameSettings::Team::Water)
                    bs.spell = 1;
                else
                    bs.spell = 0;

                state.bullets.push_back(bs);
            }

            m_host_session->send_world_state(state);
        }
    }

    // client still receives only first 2 slots for now
    if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (m_client_session)
        {
            while (true)
            {
                const auto state = m_client_session->poll_world_state();
                if (!state.has_value())
                    break;

                m_latest_world_state = *state;
            }

            if (m_latest_world_state.has_value() && hostPlayer && clientPlayer)
            {
                const auto newHostTeam = decode_team(m_latest_world_state->p1_team);
                const auto newClientTeam = decode_team(m_latest_world_state->p2_team);

                if (newHostTeam != hostPlayer->team)
                {
                    hostPlayer->team = newHostTeam;
                    if (is_combat_team(hostPlayer->team))
                        apply_team_visuals(hostPlayer->entity, hostPlayer->team);
                }

                if (newClientTeam != clientPlayer->team)
                {
                    clientPlayer->team = newClientTeam;
                    if (is_combat_team(clientPlayer->team))
                        apply_team_visuals(clientPlayer->entity, clientPlayer->team);
                }

                hostPlayer->entity.set_position(m_latest_world_state->p1_pos);
                clientPlayer->entity.set_position(m_latest_world_state->p2_pos);

                hostPlayer->entity.set_facing_dir(m_latest_world_state->p1_dir);
                clientPlayer->entity.set_facing_dir(m_latest_world_state->p2_dir);

                m_fire_kills = m_latest_world_state->fire_kills;
                m_water_kills = m_latest_world_state->water_kills;

                m_bullets.clear();

                for (const auto& b : m_latest_world_state->bullets)
                {
                    Bullet::SpellType spell =
                        (b.spell == 0) ? Bullet::SpellType::Fire : Bullet::SpellType::Water;

                    m_bullets.emplace_back(b.pos, b.dir, b.owner, spell);
                }
            }
        }
    }

    // HUD from player list
    std::ostringstream ss;

    std::string localName = "Player";
    std::string localTeam = "Spectator";
    std::string otherSummary = "None";

    if (localPlayer)
    {
        localName = localPlayer->nickname;
        localTeam = team_to_string(localPlayer->team);
    }

    std::vector<std::string> others;
    for (const auto& p : m_players)
    {
        if (!p.connected || p.id == m_local_player_id)
            continue;

        others.push_back(p.nickname + " [" + team_to_string(p.team) + "]");
    }

    if (!others.empty())
    {
        otherSummary.clear();
        for (std::size_t i = 0; i < others.size(); ++i)
        {
            if (i > 0) otherSummary += ", ";
            otherSummary += others[i];
        }
    }

    ss << "Fire: " << m_fire_kills
        << "   |   Water: " << m_water_kills
        << "   (First to " << m_kills_to_win << ")\n"
        << "You: " << localName << " [" << localTeam << "]\n"
        << "Others: " << otherSummary;

    m_hud.setString(ss.str());

    // win condition
    if (m_fire_kills >= m_kills_to_win || m_water_kills >= m_kills_to_win)
    {
        RequestStackClear();
        RequestStackPush(StateID::kGameOver);
        return false;
    }

    return true;
}