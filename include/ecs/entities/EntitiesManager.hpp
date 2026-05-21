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
        EntityWrapper Create(const PrefabEntity& prefabEntity);

        /**
         * @brief Creates new entity from prefab data (rvalue reference).
         * Moves prefab data into storage for efficiency.
         * @param prefabEntity Prefab containing component data to initialize entity
         * @return Wrapper object providing safe access to the created entity
         */
        EntityWrapper Create(PrefabEntity&& prefabEntity);

        /**
         * @brief Destroys entity if it exists.
         * Marks entity as dead, calls component destructors, and returns
         * entity ID to free list for reuse with incremented version.
         * @param entity Entity to destroy (by value)
         */
        void Destroy(const Entity& entity);

        /**
         * @brief Destroys entity using wrapper.
         * Convenience overload for EntityWrapper.
         * @param entity Wrapper containing entity to destroy
         */
        void Destroy(const EntityWrapper& entity);

        /**
         * @brief Checks if entity is alive and version matches.
         * Validates both ID bounds and version number to detect stale handles.
         * @param entity Entity to check
         * @return true if entity exists and version matches
         */
        [[nodiscard]] bool IsAlive(const Entity& entity) const;

        /**
         * @brief Checks if wrapped entity is alive.
         * Convenience overload for EntityWrapper.
         * @param entity Wrapper containing entity to check
         * @return true if entity exists and version matches
         */
        [[nodiscard]] bool IsAlive(const EntityWrapper& entity) const;

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
