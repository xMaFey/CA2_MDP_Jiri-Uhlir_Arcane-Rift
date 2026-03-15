// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include <string>

struct GameSettings
{
	enum class ControlScheme
	{
		WASD,
		Arrows
	};

	ControlScheme controls = ControlScheme::WASD;
};

