// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "button.hpp"
#include "fontID.hpp"
#include "utility.hpp"

gui::Button::Button(const FontHolder& fonts, const TextureHolder& textures)
    : m_normal_texture(textures.Get(TextureID::kButtonNormal))
    , m_selected_texture(textures.Get(TextureID::kButtonSelected))
    , m_activated_texture(textures.Get(TextureID::kButtonActivated))
    , m_text(fonts.Get(FontID::kMain), "", 16)
    , m_is_toggle(false)
    , m_sprite(textures.Get(TextureID::kButtonNormal))
{
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_text.setPosition(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
}

void gui::Button::SetCallback(Callback callback)
{
    m_callback = std::move(callback);
}

void gui::Button::SetToggle(bool flag)
{
    m_is_toggle = flag;
}

void gui::Button::SetText(const std::string& text)
{
    m_text.setString(text);
    Utility::CentreOrigin(m_text);
}

bool gui::Button::IsSelectable() const
{
    return true;
}

void gui::Button::Select()
{
    Component::Select();
    m_sprite.setTexture(m_selected_texture);
}

void gui::Button::Deselect()
{
    Component::Deselect();
    m_sprite.setTexture(m_normal_texture);
}

void gui::Button::Activate()
{
    Component::Activate();
    if (m_is_toggle)
    {
        m_sprite.setTexture(m_activated_texture);
    }
    if (m_callback)
    {
        m_callback();
    }
    if (!m_is_toggle)
    {
        Deactivate();
    }
}

void gui::Button::Deactivate()
{
    Component::Deactivate();
    if (m_is_toggle)
    {
        if (IsSelected())
        {
            m_sprite.setTexture(m_selected_texture);
        }
        else
        {
            m_sprite.setTexture(m_normal_texture);
        }
    }
}

void gui::Button::HandleEvent(const sf::Event& event)
{
}

void gui::Button::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(m_sprite, states);
    target.draw(m_text, states);
}

bool gui::Button::Contains(sf::Vector2f point_in_parent_space) const
{
	const sf::Vector2f local = getInverseTransform().transformPoint(point_in_parent_space);
    return m_sprite.getGlobalBounds().contains(local);
}