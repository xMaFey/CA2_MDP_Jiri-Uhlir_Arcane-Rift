// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "music_player.hpp"

bool MusicPlayer::PlayLoop(const std::string& filename, float volume)
{
    if (m_current == filename && m_music.getStatus() == sf::Music::Status::Playing)
        return true;

    m_music.stop();
    m_current.clear();

    if (!m_music.openFromFile(filename))
        return false;

    m_current = filename;
    m_music.setLooping(true);
    m_music.setVolume(volume);
    m_music.play();
    return true;
}

void MusicPlayer::Stop()
{
    m_music.stop();
    m_current.clear();
}

void MusicPlayer::SetVolume(float volume)
{
    m_music.setVolume(volume);
}

