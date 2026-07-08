#ifndef ENTITIES_ARCHETYPE_STORAGE_HPP
#define ENTITIES_ARCHETYPE_STORAGE_HPP

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include "../components/Utils.hpp"
#include "Archetype.hpp"
#include "ArchetypedChunks.hpp"
#include "Utils.hpp"
#include "common_recs/utils/ClassUtils.hpp"


namespace ecs
{

    /**
     * @brief Main storage for all entities across all archetypes.
     * Manages multiple ArchetypedChunks instances and provides unified entity operations.
     * Entities are identified by ArchetypedChunkEntityLocation (archetype + index within archetype).
     */
    class EntitiesArchetypeStorage final
    {
    public:
        // ==================================== EntitiesArchetypeStorage::iterator ====================================

        /**
         * @brief Forward iterator for iterating over entities with specific component requirements.
         * Skips entities that don't have all required components or are dead.
         * @tparam ValueType Type returned on dereference (tuple of components or location)
         * @tparam ComponentCls Component types that entities must have
         */
        template<typename ValueType, IsComponent... ComponentCls>
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;    ///< Iterator category compatible with forward iterator requirements.
            using value_type = ValueType;                           ///< Value returned by operator*().
            using difference_type = std::ptrdiff_t;                 ///< Signed type used for iterator distance calculations.

            /**
             * @brief Archetype composed from the required ComponentCls types.
             *
             * Used to quickly determine whether a storage archetype contains all
             * components required by this iterator. Optional `Component*` arguments are
             * excluded from the filter (see Archetype::GetViewArchetype).
             */
            static inline const Archetype s_archetype = Archetype::GetViewArchetype<ComponentCls...>();

            /**
             * @brief Constructs end iterator.
             *
             * Creates iterator in invalid/end state.
             */
            constexpr iterator() = default;

            /**
             * @brief Constructs iterator positioned at the first matching entity.
             *
             * Searches through archetypes until it finds the first archetype that
             * contains all requested component types and has at least one entity.
             *
             * @param storage Owning storage to iterate over.
             *
             * @pre storage != nullptr
             */
            explicit iterator(EntitiesArchetypeStorage* storage);

            /**
             * @brief Returns current iterator value.
             *
             * If component types were specified, returns a tuple of references to
             * requested components. Otherwise returns entity location/index value.
             *
             * @return Current iterator value.
             */
            value_type operator*() const;

            /**
             * @brief Advances iterator to the next matching entity.
             *
             * Automatically skips archetypes that do not contain all required
             * components or contain no entities.
             *
             * @return Reference to this iterator.
             */
            iterator& operator++();

            /**
             * @brief Post-increment operator.
             *
             * @return Copy of iterator before increment.
             */
            iterator operator++(int);

            /**
             * @brief Compares two iterators for equality.
             *
             * Iterators are equal when they reference the same archetype iterator
             * position and archetype index.
             *
             * @param other Iterator to compare against.
             * @return true if iterators refer to the same position.
             */
            bool operator==(const iterator& other) const;

            /**
             * @brief Compares two iterators for inequality.
             *
             * @param other Iterator to compare against.
             * @return true if iterators refer to different positions.
             */
            bool operator!=(const iterator& other) const;

        private:
            /**
             * @brief Internal iterator advancement routine.
             *
             * Advances within current archetype and automatically switches to the
             * next compatible archetype when the current one is exhausted.
             */
            void advance();

            using archetypedChunksIt_t = ArchetypedChunks::iterator<ValueType, ComponentCls...>;     ///< Iterator type used for traversal inside a single archetype storage.

            archetypedChunksIt_t m_archetypedChunksIt;                          ///< Current iterator within the active ArchetypedChunks instance.
            archetypeIndex_t m_archetypeIndex = INVALID_ARCHETYPE_INDEX;        ///< Index of currently traversed archetype.
            EntitiesArchetypeStorage* m_storage = nullptr;                      ///< Storage being iterated.
        };

        // ================================== EntitiesArchetypeStorage::chunk_iterator ==================================

        /**
         * @brief Forward iterator over the chunks of every matching archetype.
         *
         * Where iterator visits one entity at a time, chunk_iterator visits one chunk at a
         * time, dereferencing to a ChunkView over that chunk's alive entities. Empty chunks
         * and archetypes that do not contain all requested components are skipped. The total
         * chunk count is not known up front (it is discovered by walking), so the range is
         * forward-only — but each yielded chunk is an independent, self-contained unit of
         * work, which is what a parallel scheduler partitions over.
         *
         * @tparam ComponentCls Components exposed through each ChunkView.
         */
        template<IsComponent... ComponentCls>
        class chunk_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;    ///< Forward iterator: chunk count is discovered while walking.
            using value_type = ChunkView<ComponentCls...>;          ///< Value produced per chunk.
            using difference_type = std::ptrdiff_t;                 ///< Signed distance type.
            using reference = value_type;                           ///< Dereference returns a ChunkView by value.
            using pointer = void;                                   ///< No pointer indirection.

            /**
             * @brief Archetype composed from all requested ComponentCls types.
             */
            static inline const Archetype s_archetype = Archetype::GetArchetype<ComponentCls...>();

            /**
             * @brief Constructs end iterator.
             */
            constexpr chunk_iterator() = default;

            /**
             * @brief Constructs iterator positioned at the first non-empty matching chunk.
             * @param storage Owning storage to iterate over.
             * @pre storage != nullptr
             */
            explicit chunk_iterator(EntitiesArchetypeStorage* storage);

            /**
             * @brief Returns a ChunkView over the current chunk.
             */
            value_type operator*() const;

            /**
             * @brief Advances to the next non-empty matching chunk.
             */
            chunk_iterator& operator++();

            /**
             * @brief Post-increment operator.
             */
            chunk_iterator operator++(int);

            bool operator==(const chunk_iterator& other) const;
            bool operator!=(const chunk_iterator& other) const;

        private:
            /**
             * @brief Scans forward from the current position to the next non-empty
             *        chunk in a matching archetype, entering end state when exhausted.
             */
            void seekFromCurrent();

            EntitiesArchetypeStorage* m_storage = nullptr;                  ///< Storage being iterated.
            archetypeIndex_t m_archetypeIndex = INVALID_ARCHETYPE_INDEX;    ///< Index of currently traversed archetype.
            chunkEntityIndex_t m_chunkIndex = INVALID_CHUNK_ENTITY_INDEX;   ///< Index of the current chunk within the archetype.
        };

        /**
         * @brief Creates iterator over entities containing all specified components.
         *
         * Iteration spans all archetypes whose component set is a superset of
         * ComponentCls....
         *
         * @tparam Args Required component types.
         * @return Iterator positioned at the first matching entity.
         */
        template<typename... Args>
        [[nodiscard]] auto begin();

        /**
         * @brief Returns iterator representing end of traversal.
         *
         * @tparam Args Component filter type list.
         * @return End iterator.
         */
        template<typename... Args>
        [[nodiscard]] auto end() const;

        /**
         * @brief Returns a chunk iterator positioned at the first matching, non-empty chunk.
         * @tparam Args Required component types.
         * @return Begin chunk iterator.
         */
        template<typename... Args>
        [[nodiscard]] auto chunksBegin();

        /**
         * @brief Returns the end chunk iterator.
         * @tparam Args Required component types.
         * @return End chunk iterator.
         */
        template<typename... Args>
        [[nodiscard]] auto chunksEnd() const;

        // ========================================= EntitiesArchetypeStorage =========================================

        /**
         * @brief Creates a new entity from prefab data.
         * Automatically determines or finds appropriate archetype based on prefab's components.
         * @param prefabEntity Prefab containing component data
         * @param entityHandle Entity handle (id + version)
         * @param extraTagId Optional zero-sized tag bit to fold into the entity's archetype on
         *        top of the prefab's own components. Used to stamp a named EntityWrapper's own
         *        tag onto entities created through Create<Wrapper>. INVALID_COMPONENT_ID adds nothing.
         * @return Location descriptor for created entity
         */
        template<PrefabEntityRef PrefabRef>
        ArchetypedChunkEntityLocation Create(PrefabRef&& prefabEntity, Entity entityHandle, componentId_t extraTagId = INVALID_COMPONENT_ID);

        /**
         * @brief Result of migrating an entity between archetypes.
         */
        struct EntityMigration final
        {
            ArchetypedChunkEntityLocation newLocation {};       ///< Location of the entity in the new archetype storage.
            Entity swapRemovedEntity {}; ///< Entity relocated inside the old storage by swap-remove, or invalid Entity.
        };

        /**
         * @brief Returns the archetype stored at the given archetype index.
         * @param archetypeIndex Index into archetype storage
         * @return Const reference to the archetype
         */
        [[nodiscard]] const Archetype& getArchetype(archetypeIndex_t archetypeIndex) const;

        /**
         * @brief Migrates an entity from its current archetype to an arbitrary target archetype.
         *
         * Reserves a slot in the destination archetype (found or created on demand) and
         * move-constructs every component shared by both archetypes into the new slot.
         * Components present only in the old archetype (i.e. being removed) are not moved;
         * components present only in the new archetype (i.e. being added) are left
         * raw/uninitialized for the caller to construct. The entity is then removed from its
         * previous storage via swap-remove, which destructs all of its old components
         * (the moved-from shared ones and the dropped ones alike).
         *
         * @param oldLocation Current location of the entity
         * @param newArchetype Fully resolved target archetype (with an up-to-date hash)
         * @param entityHandle Entity handle (id + version) of the migrated entity
         * @return Location in the new archetype plus the swap-removed Entity
         *
         * @pre newArchetype must be non-empty and differ from the entity's current archetype.
         */
        EntityMigration MigrateEntity(
            const ArchetypedChunkEntityLocation& oldLocation, const Archetype& newArchetype, Entity entityHandle
            );

        /**
         * @brief Destroys entity at given location.
         * Calls destructors and marks slot for reuse.
         * @param entityLocation Location of entity to destroy
         * @return Entity of the swap-removed entity, or invalid Entity if none
         */
        Entity Destroy(const ArchetypedChunkEntityLocation& entityLocation);

        /**
         * @brief Gets mutable pointer to component data by component ID.
         * @param entityLocation Location of entity
         * @param componentId ID of requested component
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] byte* GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, componentId_t componentId);

        /**
         * @brief Gets const pointer to component data by component ID.
         * @param entityLocation Location of entity
         * @param componentId ID of requested component
         * @return Const pointer to component data, or nullptr if entity dead or missing component
         */
        [[nodiscard]] const byte* GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, componentId_t componentId) const;

        /**
         * @brief Attempts to get component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @param location Location of entity
         * @return Pointer to component, or nullptr if not present
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls* TryGetComponent(const ArchetypedChunkEntityLocation& location);

        /**
         * @brief Attempts to get const component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @param location Location of entity
         * @return Const pointer to component, or nullptr if not present
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls* TryGetComponent(const ArchetypedChunkEntityLocation& location) const;

        /**
         * @brief Gets component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @param location Location of entity
         * @return Reference to component
         * @note Asserts that entity has this component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls& GetComponent(const ArchetypedChunkEntityLocation& location);

        /**
         * @brief Gets const component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @param location Location of entity
         * @return Const reference to component
         * @note Asserts that entity has this component
         */
        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls& GetComponent(const ArchetypedChunkEntityLocation& location) const;

    DEEP_TEST_PRIVATE_ACCESS:
        /**
         * @brief Finds the storage index for an archetype, creating it if absent.
         * @param archetype Archetype to look up (must have an up-to-date hash)
         * @return Index of the archetype storage
         */
        archetypeIndex_t findOrCreateArchetype(const Archetype& archetype);

        /**
         * @brief Maps archetype hash to internal storage index.
         *
         * Allows constant-time lookup of ArchetypedChunks storage
         * for a particular archetype.
         */
        std::unordered_map<archetypeHash_t, archetypeIndex_t> m_archetypeIndexByHash;

        /**
         * @brief Storage containers grouped by archetype.
         *
         * Indexes correspond to values stored in m_archetypeIndexByHash.
         */
        std::vector<ArchetypedChunks> m_storageByArchetypeIndex;
    };

} // namespace ecs
#endif
#include "detail/EntitiesArchetypeStorage.ipp"
