// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include <SFML/Audio/Music.hpp>
#include <string>


class MusicPlayer
{
public:
	bool PlayLoop(const std::string& filename, float volume = 30.f);
	void Stop();
	void SetVolume(float volume);

private:
	sf::Music m_music;
	std::string m_current;
};

