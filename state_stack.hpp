// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
// Manages which state is active using a STACK.
// Top of stack = currently active state.
// Allows Pause_state to sit on top of Play_state.

#include <SFML/Graphics.hpp>
#include <functional>     
#include <memory>         
#include <typeindex>     
#include <unordered_map>  
#include <vector>         

#include "state.hpp"

class State_stack
{
public:
    // We queue these actions so we don't modify the stack mid-update/event
    enum class Action { push, pop, clear };

    // Needs the render window reference for drawing and window-size queries
    explicit State_stack(sf::RenderWindow& window);

    // Provides access to the window
    sf::RenderWindow& window();

    // Stores a factory function so we can create the state later
    template <typename TState>
    void register_state()
    {
        m_factories[typeid(TState)] = [this]()
            {
                return std::make_unique<TState>(*this);
            };
    }

    // Adds a request to push a new state on the stack
    template <typename TState>
    void push()
    {
        request_push(typeid(TState));
    }

    // Remove the top state
    void pop();

    // Remove all states
    void clear();

    // Forward calls to the top (active) state
    void handle_event(const sf::Event& e);
    void update(sf::Time dt);

    // Render all states from bottom->top
    // Needed for pause overlay (draw play then overlay)
    void render();

    bool empty() const;

private:
    // Pending stack change applied after event/update
    struct pending_change
    {
        Action action;
        std::type_index type{ typeid(void) };
    };

    void request_push(std::type_index type);
    void apply_pending_changes();

    sf::RenderWindow& m_window;
    std::vector<std::unique_ptr<State>> m_stack;

    // Maps state type -> function that creates it
    std::unordered_map<std::type_index, std::function<std::unique_ptr<State>()>> m_factories;

    // Queue of push/pop/clear requests
    std::vector<pending_change> m_pending;
};
