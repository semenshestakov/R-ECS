#ifndef ARCHETYPED_CHUNKS_HPP
#define ARCHETYPED_CHUNKS_HPP
#include "Archetype.hpp"
#include "ecs/utils/ComponentUtils.hpp"
#include "ecs/utils/EntitiesUtils.hpp"


namespace ecs
{

    struct PrefabEntity;
    struct Entity;

    /**
     * @brief Value type returned by iterator when components are requested.
     * Tuple of references to requested components.
     */
    template<IsComponent... ComponentCls>
    using iter_value_type = std::conditional_t<
        (sizeof...(ComponentCls) > 0),
        std::tuple<ComponentCls&...>,
        chunkEntityIndex_t
    >;


    /**
     * @brief Manages chunked storage for entities sharing the same archetype.
     * Organizes component data in fixed-size chunks (MAX_ENTITIES_IN_CHUNK) for cache efficiency.
     */
    struct ArchetypedChunks final
    {
        // ======================================== ArchetypedChunks::iterator =========================================

        /**
         * @brief Forward iterator over entities stored inside chunked archetype storage.
         *
         * Traverses entities sequentially across all chunks belonging to the
         * archetype and optionally exposes references to selected components.
         *
         * @tparam ValueType Type returned by operator*().
         * @tparam ComponentCls Components exposed by the iterator.
         */
        template<typename ValueType, IsComponent... ComponentCls>
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;        ///< Iterator category compatible with standard forward iterators.
            using value_type = ValueType;                               ///< Value returned by operator*().
            using difference_type = std::ptrdiff_t;                     ///< Signed type used for iterator distance calculations.

            /**
             * @brief Constructs end iterator.
             *
             * Creates iterator in invalid/end state.
             */
            constexpr iterator() = default;

            /**
             * @brief Constructs iterator positioned at the first entity.
             *
             * Initializes component array pointers and locates the first valid
             * entity in chunk storage.
             *
             * @param archetypedChunks Storage to iterate.
             *
             * @pre archetypedChunks != nullptr
             */
            explicit iterator(ArchetypedChunks* archetypedChunks);

            /**
             * @brief Returns current iterator value.
             *
             * Returns either:
             * - tuple of references to requested components;
             * - chunkEntityIndex_t when no component types were specified.
             *
             * @return Current iterator value.
             */
            value_type operator*() const;

            /**
             * @brief Advances iterator to the next entity.
             *
             * Automatically switches chunks when the current chunk is exhausted.
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
             * @brief Compares iterators for equality.
             *
             * @param other Iterator to compare against.
             * @return true if both iterators reference the same entity position.
             */
            bool operator==(const iterator& other) const;

            /**
             * @brief Compares iterators for inequality.
             *
             * @param other Iterator to compare against.
             * @return true if iterators reference different positions.
             */
            bool operator!=(const iterator& other) const;

            /**
             * @brief Checks whether iterator is valid.
             *
             * @return true if iterator points to an entity, false if it is in end state.
             */
            explicit operator bool() const;

        private:
            /**
             * @brief Advances iterator to the next entity.
             *
             * Skips exhausted chunks and enters end state when all chunks
             * have been traversed.
             */
            void advance();

            chunkEntityIndex_t m_chunkIndex = INVALID_CHUNK_ENTITY_INDEX;       ///< Current chunk index.
            chunkEntityIndex_t m_entityIndex = INVALID_CHUNK_ENTITY_INDEX;      ///< Current entity index within the chunk.
            std::tuple<ComponentCls*...> m_componentArrays;                     ///< Component array pointers for the current chunk.
            ArchetypedChunks* m_archetypedChunks = nullptr;                     ///< Chunk storage being iterated.
        };

        ArchetypedChunks() = delete;

        /**
         * @brief Constructs chunk storage for a specific archetype.
         *
         * Initializes the storage structures for entities belonging to this archetype.
         * The storage is organized by component ID with chunked memory layout for cache efficiency.
         *
         * @param a_archetype The archetype definition for entities to be stored
         *
         * @pre The archetype must have at least one component ID
         * @post Component chunks vector is sized to accommodate the largest component ID
         */
        explicit ArchetypedChunks(Archetype a_archetype);

        // Delete Copy
        ArchetypedChunks(const ArchetypedChunks& other) = delete;
        ArchetypedChunks& operator=(const ArchetypedChunks& other) = delete;

        // Use default Move
        ArchetypedChunks(ArchetypedChunks&&) noexcept = default;
        ArchetypedChunks& operator=(ArchetypedChunks&&) noexcept = default;

        /**
         * @brief Creates a new entity in this archetype's storage.
         *
         * Allocates a new entity slot, either reusing a freed index or expanding the storage.
         * Copies component data from the prefab entity into the appropriate chunk locations.
         *
         * @param entity PrefabEntity containing all component data to initialize the new entity
         * @param entityId global entity id
         * @return chunkEntityIndex_t Unique index identifying this entity within the archetype
         *
         * @pre Entity must have all components required by this archetype
         * @post Entity is marked as alive and its component data is stored in the chunk arrays
         */
        chunkEntityIndex_t Create(const PrefabEntity& entity, entityId_t entityId);

        /**
         * @brief Destroys an entity and frees its storage slot.
         *
         * Calls component destructors for all components of the entity and marks the slot as free
         * for future reuse. The memory is not immediately freed but will be reused by subsequent Create calls.
         *
         * @param chunkEntityIndex Index of the entity to destroy
         * @return global entity in chunk migration
         *
         * @pre Entity must be alive at the given index
         * @post Entity's slot is added to free list and marked as not alive
         */
        [[nodiscard]] entityId_t Destroy(chunkEntityIndex_t chunkEntityIndex);

        /**
         * @brief Retrieves mutable component data for a specific component type.
         *
         * @param chunkEntityIndex Index of the entity
         * @param componentId ID of the component to retrieve
         * @return byte* Pointer to component data, or nullptr if entity is dead
         *
         * @pre Entity must have the requested component type (asserted by caller)
         */
        [[nodiscard]] byte* GetComponentData(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId);
        [[nodiscard]] const byte* GetComponentData(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId) const;

        /**
         * @brief Attempts to get component of specified type.
         * @tparam ComponentCls Component type to retrieve
         * @param chunkEntityIndex Local entity index
         * @return Pointer to component data, or nullptr if entity dead or missing component
         */
        template<IsComponent ComponentCls> [[nodiscard]] ComponentCls* TryGetComponent(chunkEntityIndex_t chunkEntityIndex);
        template<IsComponent ComponentCls> [[nodiscard]] const ComponentCls* TryGetComponent(chunkEntityIndex_t chunkEntityIndex) const;

        /**
         * @brief Gets component of specified type (asserts existence).
         * @tparam ComponentCls Component type to retrieve
         * @param chunkEntityIndex Local entity index
         * @return Reference to component data
         * @note Asserts that entity is alive and has the component
         */
        template<IsComponent ComponentCls> [[nodiscard]] ComponentCls& GetComponent(chunkEntityIndex_t chunkEntityIndex);
        template<IsComponent ComponentCls> [[nodiscard]] const ComponentCls& GetComponent(chunkEntityIndex_t chunkEntityIndex) const;

        /**
         * @brief Gets the archetype definition for stored entities.
         * @return Const reference to archetype
         */
        [[nodiscard]] const Archetype& archetype() const;

        /**
         * @brief Returns iterator positioned at the first entity.
         *
         * @tparam ValueType Iterator value type.
         * @tparam ComponentCls Components exposed through iteration.
         *
         * @return Begin iterator.
         */
        template<typename ValueType, IsComponent... ComponentCls>
        auto begin();

        /**
         * @brief Returns iterator representing end of traversal.
         *
         * @tparam ValueType Iterator value type.
         * @tparam ComponentCls Components exposed through iteration.
         *
         * @return End iterator.
         */
        template<typename ValueType, IsComponent... ComponentCls>
        auto end();

    private:
        using componentChunks_t = std::vector<std::unique_ptr<byte[]>>;                             ///< Collection of memory chunks storing one component type.

        Archetype m_archetype;                                                                      ///< Archetype shared by all entities stored in this container.
        std::vector<componentChunks_t> m_chunksByComponentId {};                                    ///< Indexed by component ID. Each entry stores memory chunks for that component.
        std::vector<chunkEntityIndex_t> m_chunksEntityCount {};                                     ///< Number of alive entities stored in each chunk.
        collection::BitSet m_hasFreeEntityInChunk;                                                  ///< Tracks chunks that still have free capacity for new entities.

        /// @brief Maps [chunkIndex][localEntityIndex] to global entity ID
        /// Used during entity destruction to retrieve the global ID of the entity being removed.
        std::vector<std::array<entityId_t, MAX_ENTITIES_IN_CHUNK>> m_localIndexToEntityId;
    };

} // namespace ecs
#endif
#include "detail/ArchetypedChunks.ipp"