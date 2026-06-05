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
        template<typename ValueType, IsComponent... ComponentCls>
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;

            constexpr iterator() = default;

            explicit iterator(ArchetypedChunks* archetypedChunks);

            value_type operator*() const;

            iterator& operator++();

            iterator operator++(int);

            bool operator==(const iterator& other) const;

            bool operator!=(const iterator& other) const;

            explicit operator bool() const;

        private:
            void advance();

            chunkEntityIndex_t m_chunkIndex = INVALID_CHUNK_ENTITY_INDEX;
            chunkEntityIndex_t m_entityIndex = INVALID_CHUNK_ENTITY_INDEX;
            std::tuple<ComponentCls*...> m_componentArrays;
            ArchetypedChunks* m_archetypedChunks {};
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

        template<typename ValueType, IsComponent... ComponentCls>
        auto begin();

        template<typename ValueType, IsComponent... ComponentCls>
        auto end();

    private:
        using componentChunks_t = std::vector<std::unique_ptr<byte[]>>;

        Archetype m_archetype;                                                  ///< Archetype definition
        std::vector<componentChunks_t> m_chunksByComponentId {};                ///< Per-component: vector of chunk pointers
        std::vector<chunkEntityIndex_t> m_chunksEntityCount {};
        collection::BitSet m_hasFreeEntityInChunk;

        std::vector<std::array<entityId_t, MAX_ENTITIES_IN_CHUNK>> m_localIndexToEntityId;
    };

} // namespace ecs
#endif
#include "detail/ArchetypedChunks.ipp"