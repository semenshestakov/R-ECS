#pragma once
#include <functional>
#include "../Entity.hpp"


namespace ecs
{
    struct IEntitiesManager;
}

namespace ecs::ranges
{

    struct EntitiesIterator
    {
        EntitiesIterator();
        explicit EntitiesIterator(entityOpt_t value);
        explicit EntitiesIterator(entityOpt_t value, std::function<void(EntitiesIterator&)>&& funcInc);

        [[nodiscard]] bool operator==(const EntitiesIterator& other) const;
        [[nodiscard]] bool operator!=(const EntitiesIterator& other) const;

        [[nodiscard]] bool operator==(const entityOpt_t& entityOpt) const;
        [[nodiscard]] bool operator!=(const entityOpt_t& entityOpt) const;

        EntitiesIterator& operator++();
        EntitiesIterator operator++(int);
        entityId_t operator*() const;

    private:
        entityOpt_t m_value = entityNull;
        const std::function<void(EntitiesIterator&)> m_inc = nullptr;

        friend IEntitiesManager;
    };


    template<typename F>
    auto operator|(EntitiesIterator&& range, F&& func)
    {
        return func(range);
    }

}
