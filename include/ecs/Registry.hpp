#pragma once

#include "components/Components.hpp"
#include "components/ComponentsManager.hpp"
#include "entities/EntitiesManager.hpp"
#include "entities/ranges/EntitiesViews.hpp"
#include "systems/SystemManager.hpp"


namespace ecs
{

    class Registry final
    {
    public:
        /// Constructor - requires factory for initialization
        explicit Registry(EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager);
        explicit Registry(EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager, const SystemManager& systemManager);

        Registry() = default;                                           ///< Default construction
        ~Registry() = default;                                          ///< Default destructor
        Registry(Registry&&) noexcept = default;                        ///< Move constructible
        Registry& operator=(Registry&&) noexcept = default;             ///< Move assignable
        Registry(const Registry&) = delete;                             ///< Non-copyable
        Registry& operator=(const Registry&) = delete;                  ///< Non-copyable

#ifndef DEEP_TEST_ENABLE
    private:
#endif
        EntitiesManager m_entitiesManager;          ///< Underlying entity storage
        ComponentsManager m_componentsManager;      ///< Factory instance for component operations
        SystemManager m_systemManager;              ///< Underlying entity storage

    public:
        /// Find components for entity. Returns nullptr if not found.
        [[nodiscard]] Components* get(entityId_t entityId) const;

        /// Check if entity exists in registry
        [[nodiscard]] bool contains(entityId_t entityId) const;

        /// Get components for entity. Asserts/throws if entity not found.
        [[nodiscard]] Components& mustGet(entityId_t entityId) const;

        /**
         * Create components for given entity ID.
         *
         * @param entityId Entity identifier
         * @return Reference to created components
         * @throws error::InvalidEntityId if entity already exists
         */
        [[maybe_unused]] Components& Create(entityId_t entityId);

        /**
        * Create components with auto-generated entity ID.
        *
        * @return Reference to created components
        * @requires EntitiesManager must have generateId() method
        */
        [[maybe_unused]] Components& Create();

        void Update(std::optional<updateTag_t> updateTag = std::nullopt);

        template<typename... ComponentCls>
        ranges::view::ComponentsViews view();
    };

    template<typename... ComponentCls>
    ranges::view::ComponentsViews Registry::view()
    {
        if constexpr (sizeof...(ComponentCls) == 0)
        {
            return {m_entitiesManager.begin(), m_entitiesManager};
        }
        return {m_entitiesManager.end(), m_entitiesManager};
    }

} // namespace ecs
