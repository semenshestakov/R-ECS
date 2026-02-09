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

        Derived& operator++() { ++m_iterator; return static_cast<Derived&>(*this); }
        Derived operator++(int) { auto tmp = *this; ++m_iterator; return tmp; }

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


    struct ComponentsViews : DerivedViews<ComponentsViews>
    {
        using DerivedViews::DerivedViews;

        Components& operator*() const
        {
            return *m_manager.find(*m_iterator);
        }
    };

}