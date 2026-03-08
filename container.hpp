// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once
#include "component.hpp"
#include <vector>

namespace gui
{
	class Container : public Component
	{
	public:
		typedef std::shared_ptr<Container> Ptr;

	public:
		Container();
		void Pack(Component::Ptr component);
		virtual bool IsSelectable() const override;
		virtual void HandleEvent(const sf::Event& event) override;

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		bool HasSelection() const;
		void Select(std::size_t index);
		void SelectNext();
		void SelectPrevious();
		void UpdateSelectionFromMouse(sf::Vector2f mouse_point_in_container_space);
		void ClearSelection();

	private:
		std::vector<Component::Ptr> m_children;
		int m_selected_child;
	};
}

