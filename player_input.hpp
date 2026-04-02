// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================


#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

struct PlayerInput
{
    sf::Vector2f move{ 0.f, 0.f };
    bool shootHeld = false;
    bool dashPressed = false;
};

struct PlayerKeybinds
{
    sf::Keyboard::Scancode up = sf::Keyboard::Scancode::W;
    sf::Keyboard::Scancode down = sf::Keyboard::Scancode::S;
    sf::Keyboard::Scancode left = sf::Keyboard::Scancode::A;
    sf::Keyboard::Scancode right = sf::Keyboard::Scancode::D;
    sf::Keyboard::Scancode shoot = sf::Keyboard::Scancode::J;
    sf::Keyboard::Scancode dash = sf::Keyboard::Scancode::K;
};
