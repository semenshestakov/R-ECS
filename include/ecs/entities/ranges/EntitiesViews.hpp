#pragma once
#include "EntitiesIterator.hpp"



namespace ecs::ranges::view
{

    template<typename Derived>
    struct DerivedViews
    {
        DerivedViews() = delete;
        DerivedViews(EntitiesIterator&& iterator, EntitiesManager& manager) : m_iterator(std::move(iterator)), m_manager(manager) {}

        [[nodiscard]] bool operator==(const Derived& other) const { return m_iterator == other.m_iterator;}
        [[nodiscard]] bool operator!=(const Derived& other) const { return m_iterator != other.m_iterator;}

        [[nodiscard]] bool operator==(const EntitiesIterator& it) const { return m_iterator == it;}
        [[nodiscard]] bool operator!=(const EntitiesIterator& it) const { return m_iterator != it;}

        void increment() { ++m_iterator; }
        Derived& operator++() { static_cast<Derived&>(*this).increment(); return static_cast<Derived&>(*this); }
        Derived operator++(int) { auto tmp = *this; static_cast<Derived&>(*this).increment(); return tmp; }

        [[nodiscard]] Derived begin() noexcept { return static_cast<Derived&>(*this); }
        [[nodiscard]] EntitiesIterator end() const noexcept { return {}; }

    protected:
        EntitiesIterator m_iterator;
        EntitiesManager& m_manager;
    };


    struct EntityViews : DerivedViews<EntityViews>
    {
        using DerivedViews::DerivedViews;
        Entity operator*() const
        {
            const entityId_t id = *m_iterator;
            return {.id=id, .components=*m_manager.find(id)};
        }
    };


    template<typename... ComponentCls>
    struct ComponentsViews : DerivedViews<ComponentsViews<ComponentCls...>>
    {
    private:
        using Super = DerivedViews<ComponentsViews<ComponentCls...>>;
        static constexpr bool IsZeroComponents = sizeof...(ComponentCls) == 0;

    public:
        ComponentsViews(EntitiesIterator&& iterator, EntitiesManager& manager) :
             Super(std::move(iterator), manager)
        {
            if constexpr (!IsZeroComponents)
            {
                if (this->m_iterator == entityNull)
                    return;

                if (Components& components = *this->m_manager.find(*this->m_iterator); !components.contains<ComponentCls...>())
                    increment();
            }
        }

        std::conditional_t<IsZeroComponents, Components&, std::tuple<ComponentCls&...>> operator*()
        {
            Components& components = *this->m_manager.find(*this->m_iterator);
            if constexpr (IsZeroComponents)
                return components;
            else
                return components.view<ComponentCls...>();
        }

        void increment()
        {
            if constexpr (IsZeroComponents)
            {
                Super::increment();
            }
            else
            {
                for (;this->m_iterator != entityNull; Super::increment())
                {
                    if (Components &components = *this->m_manager.find(*this->m_iterator); components.contains<ComponentCls...>())
                    {
                        Super::increment();
                        return;
                    }
                }
            }
        }

    };

}