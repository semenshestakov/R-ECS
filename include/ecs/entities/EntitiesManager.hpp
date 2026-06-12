#ifndef ENTITIES_MANAGER_HPP
#define ENTITIES_MANAGER_HPP

#include <cassert>
#include <queue>

#include "EntitiesArchetypeStorage.hpp"
#include "Entity.hpp"
#include "EntityWrapper.hpp"


namespace ecs
{

    /**
     * @brief Central manager for all entities in the ECS system.
     * Handles entity lifecycle (creation, destruction, existence checks),
     * manages entity IDs with versioning for safe reuse, and provides
     * component access through entity handles. Each entity is identified
     * by a unique ID combined with a version number to prevent use-after-free bugs.
     *
     * Entities are stored in archetype-based storage for optimal cache locality
     * and efficient iteration. The manager maintains mappings from entity IDs
     * to their locations in archetype storage and tracks version numbers.
     */
    class EntitiesManager final
    {
    public:
        /**
         * @brief Constructs entity manager with initial capacity.
         * Pre-allocates storage for 256 entities to reduce reallocations.
         */
        EntitiesManager();

        /**
         * @brief Destroys all alive entities.
         * Cleans up component data and releases resources.
         */
        ~EntitiesManager();

        // Delete Copy
        EntitiesManager(const EntitiesManager& other) = delete;
        EntitiesManager& operator=(const EntitiesManager& other) = delete;

        // Use default Move
        EntitiesManager(EntitiesManager&&) noexcept = default;
        EntitiesManager& operator=(EntitiesManager&&) noexcept = default;

        /**
         * @brief Creates new entity from prefab data (lvalue reference).
         * Allocates entity ID, increments version counter, and stores entity
         * in appropriate archetype based on prefab's component composition.
         * @param prefabEntity Prefab containing component data to initialize entity
         * @return Wrapper object providing safe access to the created entity
         */
        template<EntityConcept ReturnType=EntityWrapper, PrefabEntityRef PrefabRef>
        ReturnType Create(PrefabRef&& prefabEntity);

        /**
         * @brief Adds (or overwrites) components on an existing entity.
         *
         * Recomputes the entity's archetype as the union of its current archetype and the
         * supplied components. If the archetype is unchanged (all supplied components are
         * already present) the components are overwritten in place. Otherwise the entity is
         * migrated to the new archetype: its existing components are move-constructed into
         * the new storage and the entity is swap-removed from its previous archetype.
         *
         * For each supplied component: a component already owned by the entity is assigned
         * (move-assigned when the argument is an rvalue, copy-assigned otherwise); a newly
         * added component is move-constructed when the argument is an rvalue and
         * copy-constructed otherwise.
         *
         * @tparam Args Component value types (deduced)
         * @param entity Entity to modify
         * @param args Component values to add or overwrite
         * @note No-op if the entity is not alive.
         */
        template<typename... Args>
        void AddComponents(const Entity& entity, Args&&... args);

        /**
         * @brief Removes components from an existing entity.
         *
         * Recomputes the entity's archetype by dropping the requested component types and
         * migrates the entity to the resulting archetype: every remaining component is
         * move-constructed into the new storage, the dropped components are destructed, and
         * the entity is swap-removed from its previous archetype.
         *
         * Requested components that the entity does not have are ignored. If none of the
         * requested components are present the call is a no-op. If every component would be
         * removed (the resulting archetype is empty) the entity is destroyed, since an
         * archetype-based entity cannot exist without components.
         *
         * @tparam Args Component types to remove
         * @param entity Entity to modify
         * @note No-op if the entity is not alive.
         */
        template<IsComponent... Args>
        void RemoveComponents(const Entity& entity);

        /**
         * @brief Destroys entity if it exists.
         * Marks entity as dead, calls component destructors, and returns
         * entity ID to free list for reuse with incremented version.
         * @param entity Entity to destroy (by value)
         */
        template<EntityConcept InputEntityType>
        void Destroy(const InputEntityType& entity);

        /**
         * @brief Checks if entity is alive and version matches.
         * Validates both ID bounds and version number to detect stale handles.
         * @param entity Entity to check
         * @return true if entity exists and version matches
         */
        template<EntityConcept InputEntityType>
        [[nodiscard]] bool IsAlive(const InputEntityType& entity) const;

        /**
         * @brief Gets component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @param entity Entity owning the component
         * @return Reference to component data
         * @note Asserts that entity is alive and has the component
         */
        template<IsComponent ComponentCls> [[nodiscard]] ComponentCls& GetComponent(const Entity& entity);

        /**
         * @brief Gets const component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @param entity Entity owning the component
         * @return Const reference to component data
         * @note Asserts that entity is alive and has the component
         */
        template<IsComponent ComponentCls> [[nodiscard]] const ComponentCls& GetComponent(const Entity& entity) const;

        /**
         * @brief Attempts to get component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @param entity Entity owning the component
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        template<IsComponent ComponentCls> [[nodiscard]] ComponentCls* TryGetComponent(const Entity& entity);

        /**
         * @brief Attempts to get const component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @param entity Entity owning the component
         * @return Const pointer to component data, or nullptr if entity dead or missing component
         */
        template<IsComponent ComponentCls> [[nodiscard]] const ComponentCls* TryGetComponent(const Entity& entity) const;

        /**
         * @brief Gets mutable component data by component ID.
         * Low-level access for generic component operations.
         * @param entity Entity owning the component
         * @param componentId ID of requested component
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] byte* GetComponentData(const Entity& entity, componentId_t componentId);

        /**
         * @brief Gets const component data by component ID.
         * Low-level access for generic component operations.
         * @param entity Entity owning the component
         * @param componentId ID of requested component
         * @return Const pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] const byte* GetComponentData(const Entity& entity, componentId_t componentId) const;

        /**
         * @brief Gets total number of alive entities.
         * @return Count of entities currently alive in the manager
         */
        [[nodiscard]] std::size_t size() const { return m_isAliveEntitiesCount; }

        /**
         * @brief Creates view over entities with specified components.
         * Returns a range that iterates only over entities containing all
         * requested component types. The view is lazily evaluated and
         * provides efficient iteration over archetype storage.
         * @tparam ComponentCls Component types that entities must have
         * @return Range object supporting begin()/end() iteration
         * @note Example: for (auto [pos, vel] : manager.view<Position, Velocity>())
         */
        template<IsComponent... ComponentCls>
        auto view();

    private:
        std::size_t m_isAliveEntitiesCount = 0;                                         ///< Number of currently alive entities
        entityId_t m_lastEntityId = INVALID_ENTITY_ID + 1;                              ///< Next ID to allocate (when free list empty)
        std::queue<Entity> m_freeEntities;                                              ///< Reusable entity IDs from destroyed entities

        EntitiesArchetypeStorage m_storage;                                             ///< Archetype-based component storage
        std::vector<entityVersion_t> m_versionByEntityIndex;                            ///< Version number for each entity ID
        std::vector<ArchetypedChunkEntityLocation> m_entitiesLocationByEntityIndex;     ///< Storage location for each entity ID

        /**
         * @brief Resizes internal arrays to accommodate more entities.
         * Called automatically when entity ID exceeds current capacity.
         * @param size New size for version and location arrays
         */
        void resize(std::size_t size);
    };

} // namespace ecs
#endif
#include "detail/EntitiesManager.ipp"
