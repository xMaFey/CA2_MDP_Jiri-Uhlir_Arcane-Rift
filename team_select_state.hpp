// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================


#pragma once

#include "state.hpp"
#include "container.hpp"
#include "button.hpp"
#include <SFML/Graphics/Text.hpp>
#include <string>
#include "host_session.hpp"
#include "client_session.hpp"
#include "network_packets.hpp"
#include <memory>
#include <optional>

class TeamSelectState : public State
{
public:
    TeamSelectState(StateStack& stack, Context context);

    void Draw(sf::RenderTarget& target) override;
    bool Update(sf::Time dt) override;
    bool HandleEvent(const sf::Event& event) override;

private:
    void refresh_text();

    bool can_join_fire_locally() const;
    bool can_join_water_locally() const;
    void update_button_states();
    void build_player_list_text();

private:
    sf::Text m_title;
    sf::Text m_mode_text;
    sf::Text m_name_text;
    sf::Text m_fire_text;
    sf::Text m_water_text;
    sf::Text m_hint;

    gui::Container m_gui;

    std::string m_nickname = "Player";

    int m_fire_count = 0;
    int m_water_count = 0;
    int m_team_limit = 10;

    std::unique_ptr<HostSession> m_host_session;
    std::unique_ptr<ClientSession> m_client_session;

    std::optional<LobbyStatePacket> m_latest_lobby_state;

    int m_local_player_id = -1;
    bool m_network_started = false;
    bool m_join_sent = false;

    sf::Text m_players_text;

    gui::Button::Ptr m_join_fire_button;
    gui::Button::Ptr m_join_water_button;
    gui::Button::Ptr m_spectate_button;
    gui::Button::Ptr m_start_button;
};
