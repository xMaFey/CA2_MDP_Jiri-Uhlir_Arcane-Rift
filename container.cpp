// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "container.hpp"
#include "button.hpp"

gui::Container::Container() : m_selected_child(-1)
{
}

void gui::Container::Pack(Component::Ptr component)
{
    m_children.emplace_back(component);

    if (!HasSelection() && component->IsSelectable())
    {
        Select(m_children.size() - 1);
    }
}

bool gui::Container::IsSelectable() const
{
    return false;
}

void gui::Container::HandleEvent(const sf::Event& event)
{
    // Mouse hover
    if (const auto* mm = event.getIf<sf::Event::MouseMoved>())
    {
        // event gives pixel coords; menus typically use default view so this matches world coords
        sf::Vector2f mouse_world(static_cast<float>(mm->position.x), static_cast<float>(mm->position.y));

        // Convert to container local space
        sf::Vector2f mouse_local = getInverseTransform().transformPoint(mouse_world);
        UpdateSelectionFromMouse(mouse_local);
    }

    // Mouse left click activates hovered
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mb->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mouse_world(static_cast<float>(mb->position.x), static_cast<float>(mb->position.y));
            sf::Vector2f mouse_local = getInverseTransform().transformPoint(mouse_world);

            UpdateSelectionFromMouse(mouse_local);

            if (HasSelection())
            {
                m_children[m_selected_child]->Activate();
            }
        }
    }

    // Keyboard behavior
    const auto* key_released = event.getIf<sf::Event::KeyReleased>();
    if (HasSelection() && m_children[m_selected_child]->IsActive())
    {
        m_children[m_selected_child]->HandleEvent(event);
    }
    else if (key_released)
    {
        if (key_released->scancode == sf::Keyboard::Scancode::W ||
            key_released->scancode == sf::Keyboard::Scancode::Up)
        {
            SelectPrevious();
        }
        else if (key_released->scancode == sf::Keyboard::Scancode::S ||
            key_released->scancode == sf::Keyboard::Scancode::Down)
        {
            SelectNext();
        }
        else if (key_released->scancode == sf::Keyboard::Scancode::Enter ||
            key_released->scancode == sf::Keyboard::Scancode::Space)
        {
            if (HasSelection())
            {
                m_children[m_selected_child]->Activate();
            }
        }
    }
}

void gui::Container::UpdateSelectionFromMouse(sf::Vector2f mouse_point_in_container_space)
{
    // Find the first selectable Button under the mouse and select it
    for (std::size_t i = 0; i < m_children.size(); ++i)
    {
        if (!m_children[i]->IsSelectable())
            continue;

        auto btn = std::dynamic_pointer_cast<gui::Button>(m_children[i]);
        if (btn && btn->Contains(mouse_point_in_container_space))
        {
            Select(i);
            return;
        }
    }

    ClearSelection();
}

void gui::Container::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    for (const Component::Ptr& child : m_children)
    {
        target.draw(*child, states);
    }
}

bool gui::Container::HasSelection() const
{
    return m_selected_child >= 0;
}

void gui::Container::Select(std::size_t index)
{
    if (index < m_children.size() && m_children[index]->IsSelectable())
    {
        if (HasSelection())
        {
            m_children[m_selected_child]->Deselect();
        }
        m_children[index]->Select();
        m_selected_child = static_cast<int>(index);
    }
}

void gui::Container::SelectNext()
{
    if (!HasSelection())
    {
        return;
    }

    int next = m_selected_child;
    do
    {
        next = (next + 1) % static_cast<int>(m_children.size());
    } while (!m_children[next]->IsSelectable());

    Select(next);
}

void gui::Container::SelectPrevious()
{
    if (!HasSelection())
    {
        return;
    }

    int prev = m_selected_child;
    do
    {
        prev = (prev + static_cast<int>(m_children.size()) - 1) % static_cast<int>(m_children.size());
    } while (!m_children[prev]->IsSelectable());

    Select(prev);
}

void gui::Container::ClearSelection()
{
    if (HasSelection())
    {
        m_children[m_selected_child]->Deselect();
        m_selected_child = -1;
    }
}
