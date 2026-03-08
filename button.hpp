// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "component.hpp"
#include "resource_identifiers.hpp"
#include <functional>

namespace gui
{
	class Button : public Component
	{
	public:
		typedef std::shared_ptr<Button> Ptr;
		typedef std::function<void()> Callback;

	public:
		Button(const FontHolder& fonts, const TextureHolder& textures);

		void SetCallback(Callback callback);
		void SetText(const std::string& text);
		void SetToggle(bool flag);
		
		bool Contains(sf::Vector2f point_in_parent_space) const;

		virtual bool IsSelectable() const override;
		virtual void Select() override;
		virtual void Deselect() override;
		virtual void Activate() override;
		virtual void Deactivate() override;
		virtual void HandleEvent(const sf::Event& event) override;

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	private:
		Callback m_callback;

		const sf::Texture& m_normal_texture;
		const sf::Texture& m_selected_texture;
		const sf::Texture& m_activated_texture;

		sf::Sprite m_sprite;
		sf::Text m_text;

		bool m_is_toggle;
	};
}

