#pragma once
#include <unordered_map>

#include "../components/Components.hpp"
#include "EntitiesManager.hpp"
#include "Entity.hpp"


namespace ecs
{

    /**
     * Storage proxy using std::unordered_map for entity-component mapping.
     * Provides O(1) average lookup with entity ID auto-generation.
     */
    class UMapEntitiesManager final : public IEntitiesManager
    {
        std::unordered_map<entityId_t, ComponentsPtr> m_data;
        entityId_t m_counter = 0;

    public:
        [[nodiscard]] ranges::EntitiesIterator begin() const override
        {
            auto it = m_data.begin();
            if (it == m_data.end())
                return {};

            auto state = std::make_shared<std::unordered_map<entityId_t, ComponentsPtr>::const_iterator>(it);
            auto end = m_data.end();

            entityId_t value = it->first;
            return ranges::EntitiesIterator(
                value,
                [state, end](ranges::EntitiesIterator& entitiesIterator)
                {
                    if (*state != end)
                        ++(*state);

                    IEntitiesManager::setValue(
                        entitiesIterator,
                        *state != end ? (*state)->first : entityNull
                        );
                });
        }

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

       auto iter()
        {
            return m_data | std::views::keys;
        }

    };
    static_assert(EntitiesManagerConcept<UMapEntitiesManager>, "UMapEntitiesManager is not requires EntitiesManagerConcept");

}
