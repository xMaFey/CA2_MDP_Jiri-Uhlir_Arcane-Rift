// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "sound_player.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>

void SoundPlayer::Load(SoundID id, const std::string& filename)
{
    sf::SoundBuffer buf;
    if (!buf.loadFromFile(filename))
        throw std::runtime_error("SoundPlayer::Load failed: " + filename);

    m_buffers[id] = std::move(buf);
}

void SoundPlayer::SetVolume(float v)
{
    m_volume = v;
}

void SoundPlayer::cleanup_stopped()
{
    m_sounds.remove_if([](const sf::Sound& s)
        {
            return s.getStatus() == sf::Sound::Status::Stopped;
        }
    );
}

void SoundPlayer::Play(SoundID id)
{
    auto it = m_buffers.find(id);
    if (it == m_buffers.end())
        return;

    cleanup_stopped();

    m_sounds.emplace_back(it->second);
    m_sounds.back().setVolume(m_volume);
    m_sounds.back().play();

    // DEBUG
    std::cout << "Play soud id:" << (int)id << "\n";
}
