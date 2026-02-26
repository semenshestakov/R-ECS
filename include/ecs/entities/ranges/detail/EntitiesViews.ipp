#pragma once
#include "../EntitiesViews.hpp"



namespace ecs::ranges::view
{

    template<typename... ComponentCls>
    ComponentsViews<ComponentCls...>::ComponentsViews(EntitiesIterator &&iterator, EntitiesManager &manager) :
        Super(std::move(iterator), manager)
    {
        if constexpr (!IsZeroComponents)
        {
            if (this->m_iterator == entitynull)
                return;

            if (Components& components = *this->m_manager.find(*this->m_iterator); !components.contains<ComponentCls...>())
                increment();
        }
    }

    template<typename... ComponentCls>
    std::conditional_t<ComponentsViews<ComponentCls...>::IsZeroComponents, Components &, std::tuple<ComponentCls &...>>
    ComponentsViews<ComponentCls...>::operator*()
    {
        Components& components = *this->m_manager.find(*this->m_iterator);
        if constexpr (IsZeroComponents)
            return components;
        else
            return components.view<ComponentCls...>();
    }

    template<typename... ComponentCls>
    void ComponentsViews<ComponentCls...>::increment()
    {
        if constexpr (IsZeroComponents)
        {
            Super::increment();
        }
        else
        {
            for (;this->m_iterator != entitynull; Super::increment())
            {
                if (Components &components = *this->m_manager.find(*this->m_iterator); components.contains<ComponentCls...>())
                {
                    Super::increment();
                    return;
                }
            }
        }
    }

} // namespace ecs::ranges::view
