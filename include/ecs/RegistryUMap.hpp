#pragma once
#include "Registry.hpp"


namespace ecs
{

    class _UnorderedMapProxy
    {
        std::unordered_map<entityId_t, ComponentsPtr> m_data;
        entityId_t m_counter = 0;

    public:
        Components* find(const entityId_t entityId) const
        {
            if (const auto it = m_data.find(entityId); it != m_data.end())
                return it->second.get();

            return nullptr;
        }

        void emplace(const entityId_t entityId, ComponentsPtr&& components)
        {
            m_data[entityId] = std::move(components);
        }

        entityId_t generateId()
        {
            return ++m_counter;
        }
    };

    using RegistryUMap = RegistryT<_UnorderedMapProxy>;

}
