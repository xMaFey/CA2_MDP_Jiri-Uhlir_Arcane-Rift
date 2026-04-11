// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "team_select_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"
#include "button.hpp"
#include "stateid.hpp"
#include <SFML/Network/IpAddress.hpp>
#include <iostream>

TeamSelectState::TeamSelectState(StateStack& stack, Context context)
    : State(stack, context)
    , m_title(context.fonts->Get(FontID::kMain))
    , m_mode_text(context.fonts->Get(FontID::kMain))
    , m_name_text(context.fonts->Get(FontID::kMain))
    , m_fire_text(context.fonts->Get(FontID::kMain))
    , m_water_text(context.fonts->Get(FontID::kMain))
    , m_hint(context.fonts->Get(FontID::kMain))
    , m_players_text(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f view_size = context.window->getView().getSize();

    m_title.setString("Team Select");
    m_title.setCharacterSize(48);
    Utility::CentreOrigin(m_title);
    m_title.setPosition({ view_size.x * 0.5f, view_size.y * 0.12f });

    m_mode_text.setCharacterSize(22);
    Utility::CentreOrigin(m_mode_text);
    m_mode_text.setPosition({ view_size.x * 0.5f, view_size.y * 0.18f });

    m_name_text.setCharacterSize(28);
    m_name_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.25f });

    m_fire_text.setCharacterSize(28);
    m_fire_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.36f });

    m_water_text.setCharacterSize(28);
    m_water_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.44f });

    m_players_text.setCharacterSize(20);
    m_players_text.setPosition({ view_size.x * 0.28f, view_size.y * 0.52f });

    m_hint.setString("Type nickname");
    m_hint.setCharacterSize(18);
    Utility::CentreOrigin(m_hint);
    m_hint.setPosition({ view_size.x * 0.5f, view_size.y * 0.62f });

    m_join_fire_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_join_fire_button->SetText("Join Fire");
    m_join_fire_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.72f });
    m_join_fire_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Fire;

            GetContext().sounds->Play(SoundID::kButton);
            refresh_text();

            if (settings.network_role == GameSettings::NetworkRole::Client && m_client_session)
            {
                TeamChangeRequestPacket request;
                request.requested_team = static_cast<int>(NetTeam::Fire);

                const bool ok = m_client_session->send_team_change_request(request);
                std::cout << "Client team change request sent (Fire): "
                    << (ok ? "YES" : "NO") << "\n";
            }
        });

    m_join_water_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_join_water_button->SetText("Join Water");
    m_join_water_button->setPosition({ view_size.x * 0.5f - 100.f, view_size.y * 0.80f });
    m_join_water_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Water;

            GetContext().sounds->Play(SoundID::kButton);
            refresh_text();

            if (settings.network_role == GameSettings::NetworkRole::Client && m_client_session)
            {
                TeamChangeRequestPacket request;
                request.requested_team = static_cast<int>(NetTeam::Water);

                const bool ok = m_client_session->send_team_change_request(request);
                std::cout << "Client team change request sent (Water): "
                    << (ok ? "YES" : "NO") << "\n";
            }
        });

    m_spectate_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_spectate_button->SetText("Spectate");
    m_spectate_button->setPosition({ view_size.x * 0.72f, view_size.y * 0.72f });
    m_spectate_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;
            settings.nickname = m_nickname;
            settings.chosen_team = GameSettings::Team::Spectator;

            GetContext().sounds->Play(SoundID::kButton);
            refresh_text();

            if (settings.network_role == GameSettings::NetworkRole::Client && m_client_session)
            {
                TeamChangeRequestPacket request;
                request.requested_team = static_cast<int>(NetTeam::Spectator);

                const bool ok = m_client_session->send_team_change_request(request);
                std::cout << "Client team change request sent (Spectator): "
                    << (ok ? "YES" : "NO") << "\n";
            }
        });

    m_start_button = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    m_start_button->SetText("Start Match");
    m_start_button->setPosition({ view_size.x * 0.72f, view_size.y * 0.64f });
    m_start_button->SetCallback([this]()
        {
            auto& settings = *GetContext().settings;

            if (settings.network_role != GameSettings::NetworkRole::Host)
                return;

            GetContext().sounds->Play(SoundID::kButton);

            if (m_latest_lobby_state.has_value() && m_host_session)
            {
                m_latest_lobby_state->match_started = true;

                for (const auto& p : m_latest_lobby_state->players)
                {
                    if (!p.connected || p.id == 0)
                        continue;

                    LobbyStatePacket personalized = *m_latest_lobby_state;
                    personalized.your_player_id = p.id;
                    m_host_session->send_lobby_state_to_player(p.id, personalized);
                }
            }

            RequestStackClear();
            RequestStackPush(StateID::kGame);
        });

    auto back = std::make_shared<gui::Button>(*context.fonts, *context.textures);
    back->SetText("Back");
    back->setPosition({ view_size.x * 0.72f, view_size.y * 0.80f });
    back->SetCallback([this]()
        {
            GetContext().sounds->Play(SoundID::kButton);

            if (GetContext().network)
                GetContext().network->disconnect();

            RequestStackPop();
        });

    m_gui.Pack(m_join_fire_button);
    m_gui.Pack(m_join_water_button);
    m_gui.Pack(m_spectate_button);
    m_gui.Pack(m_start_button);
    m_gui.Pack(back);

    refresh_text();

    auto& settings = *GetContext().settings;
    auto& network = *GetContext().network;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (network.mode() != NetworkManager::Mode::Host || !network.is_connected())
        {
            m_host_session = std::make_unique<HostSession>(network);
            m_host_session->start(settings.server_port);
        }
        else
        {
            m_host_session = std::make_unique<HostSession>(network);
        }

        m_network_started = true;
        m_local_player_id = 0;
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client)
    {
        if (network.mode() != NetworkManager::Mode::Client || !network.is_connected())
        {
            m_client_session = std::make_unique<ClientSession>(network);

            const auto ip = sf::IpAddress::resolve(settings.server_ip);
            bool ok = false;
            if (ip.has_value())
                ok = m_client_session->connect(*ip, settings.server_port);

            m_network_started = ok;
        }
        else
        {
            m_client_session = std::make_unique<ClientSession>(network);
            m_network_started = true;
        }
    }

    update_button_states();
}

bool TeamSelectState::can_join_fire_locally() const
{
    if (m_latest_lobby_state.has_value())
        return m_latest_lobby_state->can_join_fire;

    return m_fire_count < m_team_limit && m_fire_count <= m_water_count;
}

bool TeamSelectState::can_join_water_locally() const
{
    if (m_latest_lobby_state.has_value())
        return m_latest_lobby_state->can_join_water;

    return m_water_count < m_team_limit && m_water_count <= m_fire_count;
}

void TeamSelectState::update_button_states()
{
    if (m_join_fire_button)
        m_join_fire_button->SetEnabled(can_join_fire_locally());

    if (m_join_water_button)
        m_join_water_button->SetEnabled(can_join_water_locally());

    if (m_spectate_button)
        m_spectate_button->SetEnabled(true);

    auto& settings = *GetContext().settings;
    if (m_start_button)
        m_start_button->SetEnabled(settings.network_role == GameSettings::NetworkRole::Host);
}

void TeamSelectState::build_player_list_text()
{
    if (!m_latest_lobby_state.has_value())
    {
        m_players_text.setString("Players: waiting for lobby state...");
        return;
    }

    std::string text = "Players:\n";

    for (const auto& p : m_latest_lobby_state->players)
    {
        if (!p.connected)
            continue;

        std::string team = "Spectator";
        if (p.team == static_cast<int>(NetTeam::Fire)) team = "Fire";
        else if (p.team == static_cast<int>(NetTeam::Water)) team = "Water";

        text += p.nickname + " [" + team + "]\n";
    }

    m_players_text.setString(text);
}

void TeamSelectState::refresh_text()
{
    const auto& settings = *GetContext().settings;

    std::string mode = "Offline";
    if (settings.network_role == GameSettings::NetworkRole::Host)
        mode = "Mode: Host Lobby";
    else if (settings.network_role == GameSettings::NetworkRole::Client)
        mode = "Mode: Client Lobby";

    m_mode_text.setString(mode);
    Utility::CentreOrigin(m_mode_text);

    m_fire_count = settings.latest_fire_count;
    m_water_count = settings.latest_water_count;
    m_team_limit = settings.team_limit;

    m_name_text.setString("Nickname: " + m_nickname);
    m_fire_text.setString("Fire Team: " + std::to_string(m_fire_count) + "/" + std::to_string(m_team_limit));
    m_water_text.setString("Water Team: " + std::to_string(m_water_count) + "/" + std::to_string(m_team_limit));

    update_button_states();
    build_player_list_text();
}

void TeamSelectState::Draw(sf::RenderTarget& target)
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    sf::RectangleShape overlay;
    overlay.setSize(GetContext().window->getView().getSize());
    overlay.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(overlay);

    target.draw(m_title);
    target.draw(m_mode_text);
    target.draw(m_name_text);
    target.draw(m_fire_text);
    target.draw(m_water_text);
    target.draw(m_hint);
    target.draw(m_players_text);
    target.draw(m_gui);
}

bool TeamSelectState::Update(sf::Time)
{
    auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host && m_host_session)
    {
        while (true)
        {
            const auto joinInfo = m_host_session->poll_join_info();
            if (!joinInfo.has_value())
                break;

            int fireCount = 0;
            int waterCount = 0;

            if (settings.chosen_team == GameSettings::Team::Fire) ++fireCount;
            if (settings.chosen_team == GameSettings::Team::Water) ++waterCount;

            if (m_latest_lobby_state.has_value())
            {
                for (const auto& p : m_latest_lobby_state->players)
                {
                    if (!p.connected || p.id == 0)
                        continue;

                    if (p.id == joinInfo->player_id)
                        continue;

                    if (p.team == static_cast<int>(NetTeam::Fire)) ++fireCount;
                    else if (p.team == static_cast<int>(NetTeam::Water)) ++waterCount;
                }
            }

            const GameSettings::Team requested =
                (joinInfo->team == static_cast<int>(NetTeam::Fire)) ? GameSettings::Team::Fire :
                (joinInfo->team == static_cast<int>(NetTeam::Water)) ? GameSettings::Team::Water :
                GameSettings::Team::Spectator;

            GameSettings::Team finalTeam = GameSettings::Team::Spectator;

            if (requested == GameSettings::Team::Fire &&
                fireCount < settings.team_limit &&
                fireCount <= waterCount)
            {
                finalTeam = GameSettings::Team::Fire;
            }
            else if (requested == GameSettings::Team::Water &&
                waterCount < settings.team_limit &&
                waterCount <= fireCount)
            {
                finalTeam = GameSettings::Team::Water;
            }

            if (!m_latest_lobby_state.has_value())
                m_latest_lobby_state = LobbyStatePacket{};

            LobbyPlayerState player;
            player.id = joinInfo->player_id;
            player.nickname = joinInfo->nickname;
            player.team = (finalTeam == GameSettings::Team::Fire) ? static_cast<int>(NetTeam::Fire)
                : (finalTeam == GameSettings::Team::Water) ? static_cast<int>(NetTeam::Water)
                : static_cast<int>(NetTeam::Spectator);
            player.connected = true;

            bool replaced = false;
            for (auto& existing : m_latest_lobby_state->players)
            {
                if (existing.id == player.id)
                {
                    existing = player;
                    replaced = true;
                    break;
                }
            }

            if (!replaced)
                m_latest_lobby_state->players.push_back(player);
        }

        while (true)
        {
            const auto request = m_host_session->poll_team_change_request();
            if (!request.has_value())
                break;

            if (!m_latest_lobby_state.has_value())
                continue;

            int fireCount = (settings.chosen_team == GameSettings::Team::Fire) ? 1 : 0;
            int waterCount = (settings.chosen_team == GameSettings::Team::Water) ? 1 : 0;

            for (const auto& p : m_latest_lobby_state->players)
            {
                if (!p.connected || p.id == request->first)
                    continue;

                if (p.team == static_cast<int>(NetTeam::Fire)) ++fireCount;
                else if (p.team == static_cast<int>(NetTeam::Water)) ++waterCount;
            }

            std::cout << "Host got team request from player " << request->first
                << " requested=" << request->second.requested_team
                << " fireCount(excluding requester)=" << fireCount
                << " waterCount(excluding requester)=" << waterCount << "\n";

            for (auto& p : m_latest_lobby_state->players)
            {
                if (p.id != request->first)
                    continue;

                const int oldTeam = p.team;
                const int requestedTeam = request->second.requested_team;

                if (requestedTeam == static_cast<int>(NetTeam::Spectator))
                {
                    p.team = static_cast<int>(NetTeam::Spectator);
                }
                else if (requestedTeam == static_cast<int>(NetTeam::Fire))
                {
                    if (fireCount < settings.team_limit && fireCount <= waterCount)
                        p.team = static_cast<int>(NetTeam::Fire);
                }
                else if (requestedTeam == static_cast<int>(NetTeam::Water))
                {
                    if (waterCount < settings.team_limit && waterCount <= fireCount)
                        p.team = static_cast<int>(NetTeam::Water);
                }

                std::cout << "Host applied team change? old=" << oldTeam
                    << " new=" << p.team << "\n";
                break;
            }
        }

        for (int disconnectedId : m_host_session->consume_disconnected_player_ids())
        {
            if (!m_latest_lobby_state.has_value())
                continue;

            for (auto& p : m_latest_lobby_state->players)
            {
                if (p.id == disconnectedId)
                {
                    p.connected = false;
                    p.team = static_cast<int>(NetTeam::Spectator);
                }
            }
        }

        if (!m_latest_lobby_state.has_value())
            m_latest_lobby_state = LobbyStatePacket{};

        bool hostFound = false;
        for (auto& p : m_latest_lobby_state->players)
        {
            if (p.id == 0)
            {
                p.nickname = m_nickname;
                p.team = (settings.chosen_team == GameSettings::Team::Fire) ? static_cast<int>(NetTeam::Fire)
                    : (settings.chosen_team == GameSettings::Team::Water) ? static_cast<int>(NetTeam::Water)
                    : static_cast<int>(NetTeam::Spectator);
                p.connected = true;
                hostFound = true;
                break;
            }
        }

        if (!hostFound)
        {
            LobbyPlayerState hostPlayer;
            hostPlayer.id = 0;
            hostPlayer.nickname = m_nickname;
            hostPlayer.team = (settings.chosen_team == GameSettings::Team::Fire) ? static_cast<int>(NetTeam::Fire)
                : (settings.chosen_team == GameSettings::Team::Water) ? static_cast<int>(NetTeam::Water)
                : static_cast<int>(NetTeam::Spectator);
            hostPlayer.connected = true;
            m_latest_lobby_state->players.push_back(hostPlayer);
        }

        int fireCount = 0;
        int waterCount = 0;
        int spectatorCount = 0;

        for (const auto& p : m_latest_lobby_state->players)
        {
            if (!p.connected)
                continue;

            if (p.team == static_cast<int>(NetTeam::Fire)) ++fireCount;
            else if (p.team == static_cast<int>(NetTeam::Water)) ++waterCount;
            else ++spectatorCount;
        }

        m_latest_lobby_state->fire_count = fireCount;
        m_latest_lobby_state->water_count = waterCount;
        m_latest_lobby_state->spectator_count = spectatorCount;

        m_latest_lobby_state->can_join_fire =
            (fireCount < settings.team_limit && fireCount <= waterCount);
        m_latest_lobby_state->can_join_water =
            (waterCount < settings.team_limit && waterCount <= fireCount);
        m_latest_lobby_state->can_spectate = true;
        m_latest_lobby_state->match_started = false;

        settings.latest_fire_count = fireCount;
        settings.latest_water_count = waterCount;
        settings.latest_spectator_count = spectatorCount;

        refresh_text();

        for (const auto& p : m_latest_lobby_state->players)
        {
            if (!p.connected || p.id == 0)
                continue;

            LobbyStatePacket personalized = *m_latest_lobby_state;
            personalized.your_player_id = p.id;
            m_host_session->send_lobby_state_to_player(p.id, personalized);
        }
    }
    else if (settings.network_role == GameSettings::NetworkRole::Client && m_client_session)
    {
        if (m_network_started && !m_join_sent)
        {
            JoinInfoPacket joinInfo;
            joinInfo.player_id = -1;
            joinInfo.nickname = m_nickname;

            if (settings.chosen_team == GameSettings::Team::Fire)
                joinInfo.team = static_cast<int>(NetTeam::Fire);
            else if (settings.chosen_team == GameSettings::Team::Water)
                joinInfo.team = static_cast<int>(NetTeam::Water);
            else
                joinInfo.team = static_cast<int>(NetTeam::Spectator);

            m_client_session->send_join_info(joinInfo);
            m_join_sent = true;
        }

        while (true)
        {
            const auto lobby = m_client_session->poll_lobby_state();
            if (!lobby.has_value())
                break;

            m_latest_lobby_state = *lobby;
        }

        if (m_latest_lobby_state.has_value())
        {
            m_local_player_id = m_latest_lobby_state->your_player_id;

            settings.latest_fire_count = m_latest_lobby_state->fire_count;
            settings.latest_water_count = m_latest_lobby_state->water_count;
            settings.latest_spectator_count = m_latest_lobby_state->spectator_count;

            for (const auto& p : m_latest_lobby_state->players)
            {
                if (!p.connected)
                    continue;

                if (p.id != m_local_player_id)
                    continue;

                if (p.team == static_cast<int>(NetTeam::Fire))
                    settings.chosen_team = GameSettings::Team::Fire;
                else if (p.team == static_cast<int>(NetTeam::Water))
                    settings.chosen_team = GameSettings::Team::Water;
                else
                    settings.chosen_team = GameSettings::Team::Spectator;

                break;
            }

            refresh_text();

            if (m_latest_lobby_state->match_started)
            {
                RequestStackClear();
                RequestStackPush(StateID::kGame);
            }
        }
    }
    else
    {
        refresh_text();
    }

    return true;
}

bool TeamSelectState::HandleEvent(const sf::Event& event)
{
    m_gui.HandleEvent(event);

    if (const auto* text = event.getIf<sf::Event::TextEntered>())
    {
        unsigned int unicode = text->unicode;

        if (unicode >= 32 && unicode <= 126 && m_nickname.size() < 14)
        {
            m_nickname += static_cast<char>(unicode);
            GetContext().settings->nickname = m_nickname;
            refresh_text();

            auto& settings = *GetContext().settings;
            if (settings.network_role == GameSettings::NetworkRole::Client &&
                m_client_session &&
                m_network_started)
            {
                JoinInfoPacket joinInfo;
                joinInfo.player_id = -1;
                joinInfo.nickname = m_nickname;

                if (settings.chosen_team == GameSettings::Team::Fire)
                    joinInfo.team = static_cast<int>(NetTeam::Fire);
                else if (settings.chosen_team == GameSettings::Team::Water)
                    joinInfo.team = static_cast<int>(NetTeam::Water);
                else
                    joinInfo.team = static_cast<int>(NetTeam::Spectator);

                m_client_session->send_join_info(joinInfo);
                m_join_sent = true;
            }
        }
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->scancode == sf::Keyboard::Scancode::Backspace && !m_nickname.empty())
        {
            m_nickname.pop_back();
            GetContext().settings->nickname = m_nickname;
            refresh_text();

            auto& settings = *GetContext().settings;
            if (settings.network_role == GameSettings::NetworkRole::Client &&
                m_client_session &&
                m_network_started)
            {
                JoinInfoPacket joinInfo;
                joinInfo.player_id = -1;
                joinInfo.nickname = m_nickname;

                if (settings.chosen_team == GameSettings::Team::Fire)
                    joinInfo.team = static_cast<int>(NetTeam::Fire);
                else if (settings.chosen_team == GameSettings::Team::Water)
                    joinInfo.team = static_cast<int>(NetTeam::Water);
                else
                    joinInfo.team = static_cast<int>(NetTeam::Spectator);

                m_client_session->send_join_info(joinInfo);
                m_join_sent = true;
            }
        }
        else if (key->scancode == sf::Keyboard::Scancode::Escape)
        {
            if (GetContext().network)
                GetContext().network->disconnect();

            RequestStackPop();
        }
    }

    auto& settings = *GetContext().settings;

    if (settings.network_role == GameSettings::NetworkRole::Host)
    {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        {
            if (key->scancode == sf::Keyboard::Scancode::Num1 && can_join_fire_locally())
            {
                settings.chosen_team = GameSettings::Team::Fire;
                refresh_text();
            }
            else if (key->scancode == sf::Keyboard::Scancode::Num2 && can_join_water_locally())
            {
                settings.chosen_team = GameSettings::Team::Water;
                refresh_text();
            }
            else if (key->scancode == sf::Keyboard::Scancode::Num3)
            {
                settings.chosen_team = GameSettings::Team::Spectator;
                refresh_text();
            }
        }
    }

    return false;
}