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

        /**
         * @brief Updates all systems in the registry.
         *
         * @param updateTag Optional tag to control which systems should update.
         *                  If nullopt (default), all systems are updated.
         *                  If provided, only systems matching the tag are updated.
         *
         * @note This method delegates to SystemManager::Update().
         * @note Systems are updated in the order they were registered.
         * @note The registry itself is passed to systems for component access.
         *
         * Example usage:
         * @code
         * registry.Update();                    // Update all systems
         * registry.Update(updateTag_t::Physics); // Update only physics systems
         * @endcode
         *
         * @see SystemManager::Update() for detailed update semantics
         */
        void Update(std::optional<updateTag_t> updateTag = std::nullopt);

        /**
         * @brief Creates a view over entities that have all specified component types.
         *
         * @tparam ComponentCls... Variadic list of component types that entities must possess.
         * @return ranges::view::ComponentsViews<ComponentCls...> A range view for iteration.
         *
         * @note The view is lazy - it doesn't create a new container, just provides iteration.
         * @note Entities are iterated in their natural storage order.
         *
         * Example usage:
         * @code
         * for (auto [pos, vel] : registry.view<Position, Velocity>())
         *     pos.x += vel.dx;  // Direct access to component references
         *
         * for (auto& components : registry.view())
         *     components.mustGet<Position>().x += components.mustGet<Velocity>().dx // if present
         *
         * @endcode
         *
         * @see ComponentsViews for iterator implementation details
         */
        template<typename... ComponentCls>
        ranges::view::ComponentsViews<ComponentCls...> view();
    };

    template<typename... ComponentCls>
    ranges::view::ComponentsViews<ComponentCls...> Registry::view()
    {
        return {m_entitiesManager.begin(), m_entitiesManager};
    }

} // namespace ecs
