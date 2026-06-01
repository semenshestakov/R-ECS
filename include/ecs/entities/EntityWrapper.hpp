#ifndef ECS_ENTITY_WRAPPER_HPP
#define ECS_ENTITY_WRAPPER_HPP
#include <functional>
#include "Entity.hpp"
#include "ecs/utils/ComponentUtils.hpp"


namespace ecs
{
    class EntitiesManager;


    /**
     * @brief RAII-style wrapper providing safe and convenient entity access.
     * EntityWrapper combines an Entity (ID + version) with a reference to the
     * owning EntitiesManager, enabling method chaining and automatic context
     * passing. It serves as the primary interface for entity operations,     * ensuring that entity handles are always used with the correct manager.
     *
     * The wrapper is lightweight (stores entity and manager reference) and
     * is intended to be passed by value. All methods forward operations to
     * the underlying manager with proper entity context.
     *
     * @note EntityWrapper does NOT automatically destroy the entity when destroyed.
     *       Call SelfDestroy() explicitly or use the manager's Destroy method.
     */
    struct EntityWrapper
    {
        EntityWrapper() = delete;

        /**
         * @brief Constructs wrapper for an entity.
         * @param entity Entity handle to wrap (ID + version)
         * @param entitiesManager Reference to manager owning the entity
         */
        EntityWrapper(const Entity& entity, EntitiesManager& entitiesManager);

        /**
         * @brief Checks if wrapped entity is still alive.
         * Validates both entity ID bounds and version number to detect
         * stale handles from destroyed or reused entities.
         * @return true if entity exists and version matches
         */
        [[nodiscard]] bool IsAlive() const;

        /**
         * @brief Destroys the wrapped entity.
         * Removes entity from world, calls component destructors, and marks
         * entity ID for reuse with incremented version. After destruction,
         * IsAlive() will return false for this wrapper.
         * @note Does not invalidate this wrapper - it continues to exist but
         *       IsAlive() will return false and component access will fail.
         */
        void SelfDestroy() const;

        /**
         * @brief Gets component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @return Reference to component data
         * @note Asserts that entity is alive and has the component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls& GetComponent();

        /**
         * @brief Gets const component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @return Const reference to component data
         * @note Asserts that entity is alive and has the component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls& GetComponent() const;

        /**
         * @brief Attempts to get component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls* TryGetComponent();

        /**
         * @brief Attempts to get const component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @return Const pointer to component data, or nullptr if entity dead or missing component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls* TryGetComponent() const;

        /**
         * @brief Gets mutable component data by component ID.
         * Low-level access for generic component operations.
         * @param componentId ID of requested component
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] byte* GetComponentData(componentId_t componentId);

        /**
         * @brief Gets const component data by component ID.
         * Low-level access for generic component operations.
         * @param componentId ID of requested component
         * @return Const pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] const byte* GetComponentData(componentId_t componentId) const;

        /**
         * @brief Gets underlying entity handle.
         * Provides access to raw entity (ID + version) for cases where
         * direct entity reference is needed.
         * @return Const reference to stored entity
         */
        [[nodiscard]] const Entity& getEntity() const { return m_entity; }

        [[nodiscard]] entityId_t getId() const { return getEntity().id; }
        [[nodiscard]] entityVersion_t getVersion() const { return getEntity().version; }

    private:
        Entity m_entity;                                            ///< Wrapped entity handle (ID + version)
        std::reference_wrapper<EntitiesManager> m_managerRef;       ///< Reference to owning manager
    };

} // namespace ecs
#endif
#include "detail/EntityWrapper.ipp"
