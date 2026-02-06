#pragma once
#include <unordered_map>
#include "Registry.hpp"


namespace ecs
{

    /**
     * Storage proxy using std::unordered_map for entity-component mapping.
     * Provides O(1) average lookup with entity ID auto-generation.
     */
    class UnorderedMapProxy final
    {
        std::unordered_map<entityId_t, ComponentsPtr> m_data;
        entityId_t m_counter = 0;

    public:
        /**
         * Find components for entity.
         * @param entityId Entity identifier
         * @return Raw pointer to components or nullptr
         */
        [[nodiscard]] Components* find(const entityId_t entityId) const
        {
            if (const auto it = m_data.find(entityId); it != m_data.end())
                return it->second.get();

            return nullptr;
        }

        /**
         * Insert or replace components for entity.
         * @param entityId Entity identifier
         * @param components Components to store (moved)
         */
        void emplace(const entityId_t entityId, ComponentsPtr&& components)
        {
            m_data[entityId] = std::move(components);
        }

        /**
        * Generate new unique entity ID.
        * @return Next available entity ID
        */
        entityId_t generateId()
        {
            return ++m_counter;
        }
    };

    using RegistryUMap = RegistryT<UnorderedMapProxy>;

}
