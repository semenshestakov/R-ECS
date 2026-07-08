#ifndef ENTITIES_MANAGER_HPP
#define ENTITIES_MANAGER_HPP

#include <cassert>
#include <queue>

#include "EntitiesArchetypeStorage.hpp"
#include "Entity.hpp"
#include "EntityWrapper.hpp"
#include "ecs/jobs/ThreadAffinity.hpp"
#include "common_recs/utils/ClassUtils.hpp"


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
         * @brief Adds zero-sized tags to an existing entity.
         *
         * Sets the tag bits and migrates the entity to the resulting archetype without
         * constructing any component data. Tags already present are ignored; if the set
         * of tags is fully present the call is a no-op.
         *
         * @tparam Tags Tag types (must derive from ecs::Tag).
         * @param entity Entity to modify.
         * @note No-op if the entity is not alive.
         */
        template<IsTag... Tags>
        void AddTag(const Entity& entity);

        /**
         * @brief Removes zero-sized tags from an existing entity.
         *
         * Equivalent to RemoveComponents for tag types: drops the tag bits and migrates
         * the entity. Tags the entity does not have are ignored.
         *
         * @tparam Tags Tag types (must derive from ecs::Tag).
         * @param entity Entity to modify.
         * @note No-op if the entity is not alive.
         */
        template<IsTag... Tags>
        void RemoveTag(const Entity& entity);

        /**
         * @brief Checks whether an entity carries the given tag.
         *
         * Tests the tag's bit in the entity's archetype. Since a tag stores no data,
         * this is the tag counterpart of TryGetComponent.
         *
         * @tparam TagCls Tag type (must derive from ecs::Tag).
         * @param entity Entity to inspect.
         * @return true if the entity is alive and its archetype has the tag bit set.
         * @note Returns false (rather than asserting) if the entity is not alive.
         */
        template<IsTag TagCls> [[nodiscard]] bool HasTag(const Entity& entity) const;

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
         * @brief Bulk component access mirroring the view<...> element semantics.
         *
         * Each argument maps exactly like a view argument: a required `Component`
         * yields a reference (GetComponent), an optional `Component*` yields a pointer
         * that is nullptr when the entity lacks it (TryGetComponent). The result is a
         * tuple of those elements, ready for structured bindings.
         *
         * @tparam Args Component types: `Component` required, `Component*` optional.
         * @param entity Entity to read.
         * @return Tuple with one reference/pointer per argument.
         * @note Asserts the entity is alive; required components must be present.
         * @note Example: auto [pos, vel] = manager.GetComponents<Position2d, Velocity2d*>(e);
         */
        template<typename... Args> [[nodiscard]] auto GetComponents(const Entity& entity);

        /**
         * @brief Const overload of GetComponents.
         *
         * Required arguments yield `const Component&`, optional arguments yield
         * `const Component*`.
         *
         * @tparam Args Component types: `Component` required, `Component*` optional.
         * @param entity Entity to read.
         * @return Tuple with one const reference/pointer per argument.
         */
        template<typename... Args> [[nodiscard]] auto GetComponents(const Entity& entity) const;

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
         * @brief Returns the archetype an entity currently belongs to.
         *
         * The archetype is the entity's component-set fingerprint: it carries a bit for
         * every component and tag the entity owns (`archetype.test(componentId)`). It is
         * the storage's own archetype, so the reference stays valid until the entity is
         * migrated (Add/Remove Components/Tags) or destroyed.
         *
         * @param entity Entity to inspect.
         * @return Const reference to the entity's archetype.
         * @note Asserts that the entity is alive.
         * @note Example: bool frozen = manager.GetArchetype(e).test(
         *                    ComponentRegistrator::GetComponentId<Frozen>());
         */
        [[nodiscard]] const Archetype& GetArchetype(const Entity& entity) const;

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
         * @tparam Args Component types that entities must have
         * @return Range object supporting begin()/end() iteration
         * @note Example: for (auto [pos, vel] : manager.view<Position, Velocity>())
         *
         * Only the first argument (`Head`) selects the entity handle yielded alongside the
         * components:
         *  - `Entity`             yields the raw Entity (not part of the filter).
         *  - the base `EntityWrapper` yields a wrapper and adds no filter.
         *  - a named `EntityWrapper` subclass yields that wrapper and filters on its own
         *    tag bit (EntityWrapper derives from ecs::Tag, so every subclass is a tag).
         *  - a tag type (`ecs::Tag`-derived) yields the Entity and filters on the tag.
         * Plain tags anywhere in the argument list are filter-only: they contribute an
         * archetype bit but are never yielded and never allocate a column.
         *
         * Every remaining (`Tail`) argument must be a component (`Component` / `Component*`)
         * or a filter-only tag — never a second handle. Placing an `Entity` or another
         * `EntityWrapper` handle after `Head` is a compile error: a view yields exactly one
         * handle, chosen by `Head`.
         * @note Example: for (auto [e] : manager.view<Door>())  // Door : EntityWrapper (a named wrapper is a tag)
         */
        template<typename... Args>
        auto view();

        /**
         * @brief Internal: resolves the head argument of view<> to the concrete range type.
         * @tparam Head First view argument (handle selector or component).
         * @tparam Tail Remaining view arguments.
         */
        template<typename Head, typename... Tail>
        auto viewDispatch();

        /**
         * @brief Creates a chunk view over entities with specified components.
         *
         * Like view(), but iterates one chunk at a time instead of one entity at a time:
         * each element is a ChunkView over the alive entities of a single chunk. Chunks are
         * independent units of work, which makes this the entry point for data-parallel
         * iteration — a scheduler partitions the (forward, unknown-length) chunk range and
         * hands a chunk, or a run of chunks, to each worker.
         *
         * @tparam Args Component types that entities must have.
         * @return Range object supporting begin()/end() over ChunkView elements.
         * @note Example: for (auto chunk : manager.chunkView<Position, Velocity>())
         *                  for (auto [pos, vel] : chunk) { ... }
         */
        template<typename... Args>
        auto chunkView();

    DEEP_TEST_PRIVATE_ACCESS:
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
