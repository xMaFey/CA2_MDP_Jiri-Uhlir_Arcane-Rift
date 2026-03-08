// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "state_stack.hpp"

State_stack::State_stack(sf::RenderWindow& window)
    : m_window(window)
{
}

sf::RenderWindow& State_stack::window()
{
    return m_window;
}

bool State_stack::empty() const
{
    return m_stack.empty();
}

void State_stack::request_push(std::type_index type)
{
    // Queue up a push request
    m_pending.push_back({ Action::push, type });
}

void State_stack::pop()
{
    // Queue up a pop request
    m_pending.push_back({ Action::pop, typeid(void) });
}

void State_stack::clear()
{
    // Queue up a clear request
    m_pending.push_back({ Action::clear, typeid(void) });
}

void State_stack::handle_event(const sf::Event& e)
{
    // Only the top state handles input
    if (!m_stack.empty())
        m_stack.back()->handle_event(e);

    // Apply queued state changes safely AFTER the event
    apply_pending_changes();
}

void State_stack::update(sf::Time dt)
{
    // Only the top state updates (important: Pause_state freezes gameplay)
    if (!m_stack.empty())
        m_stack.back()->update(dt);

    // Apply queued state changes safely AFTER update
    apply_pending_changes();
}

void State_stack::render()
{
    // Draw bottom -> top so overlays work
    for (auto& s : m_stack)
        s->render(m_window);
}

void State_stack::apply_pending_changes()
{
    for (const auto& c : m_pending)
    {
        if (c.action == Action::clear)
        {
            while (!m_stack.empty())
            {
                m_stack.back()->on_exit();
                m_stack.pop_back();
            }
        }
        else if (c.action == Action::pop)
        {
            if (!m_stack.empty())
            {
                m_stack.back()->on_exit();
                m_stack.pop_back();
            }
        }
        else if (c.action == Action::push)
        {
            auto it = m_factories.find(c.type);
            if (it != m_factories.end())
            {
                auto s = it->second();
                s->on_enter();
                m_stack.push_back(std::move(s));
            }
        }
    }

    m_pending.clear();
}
