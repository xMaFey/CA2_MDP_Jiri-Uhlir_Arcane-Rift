// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "component.hpp"

gui::Component::Component() : m_is_selected(false), m_is_active(false), m_is_enabled(true)
{
}

gui::Component::~Component()
{
}

bool gui::Component::IsSelected() const
{
    return m_is_selected;
}

void gui::Component::Select()
{
    m_is_selected = true;
}

void gui::Component::Deselect()
{
    m_is_selected = false;
}

bool gui::Component::IsActive() const
{
    return m_is_active;
}

void gui::Component::Activate()
{
    m_is_active = true;
}

void gui::Component::Deactivate()
{
    m_is_active = false;
}

void gui::Component::SetEnabled(bool enabled)
{
    m_is_enabled = enabled;

    if (!m_is_enabled)
    {
        m_is_selected = false;
        m_is_active = false;
    }
}

bool gui::Component::IsEnabled() const
{
    return m_is_enabled;
}
