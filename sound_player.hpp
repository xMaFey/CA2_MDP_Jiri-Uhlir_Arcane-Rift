// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#include "sound_id.hpp"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <map>
#include <vector>
#include <string>


class SoundPlayer
{
public:
	void Load(SoundID id, const std::string& filename);
	void Play(SoundID id);
	void SetVolume(float v);

private:
	void cleanup_stopped();

private:
	std::map<SoundID, sf::SoundBuffer> m_buffers;
	std::list<sf::Sound> m_sounds;
	float m_volume = 85.f;
};

