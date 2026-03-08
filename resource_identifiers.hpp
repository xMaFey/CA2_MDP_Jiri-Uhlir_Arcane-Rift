// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "texture_id.hpp"
#include "resource_holder.hpp"
#include <SFML/Graphics/Font.hpp>
#include "fontID.hpp"

namespace sf
{
	class Texture;
}

//template<typename Identifier, typename Resource>

typedef ResourceHolder<TextureID, sf::Texture> TextureHolder;
typedef ResourceHolder<FontID, sf::Font> FontHolder;