#ifndef ENTITIES_ARCHETYPE_STORAGE_HPP
#define ENTITIES_ARCHETYPE_STORAGE_HPP

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>
#include "Archetype.hpp"
#include "ecs/utils/ComponentUtils.hpp"
#include "ecs/utils/EntitiesUtils.hpp"


namespace ecs
{

    struct PrefabEntity;
    struct Entity;

    using chunkEntityIndex_t = std::uint32_t;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK = 256;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK_MASK = MAX_ENTITIES_IN_CHUNK - 1;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNKS = ~0u;

    // ========================================= ArchetypedChunkEntityLocation =========================================

    /**
     * @brief Location descriptor for an entity within archetype storage.
     * Combines archetype index and chunk-local entity index to uniquely identify an entity.
     */
    struct ArchetypedChunkEntityLocation final
    {
        archetypeIndex_t archetypeIndex {};            ///< Index of the archetype in storage
        chunkEntityIndex_t chunkEntityIndex {};        ///< Global entity index within archetype chunks

        /**
         * @brief Compares two locations for equality.
         * @param other Location to compare with
         * @return true if both archetype index and chunk entity index match
         */
        [[nodiscard]] bool operator==(const ArchetypedChunkEntityLocation& other) const;
        [[nodiscard]] bool operator!=(const ArchetypedChunkEntityLocation& other) const;
    };


    // =============================================== ArchetypedChunks ===============================================

    /**
     * @brief Manages chunked storage for entities sharing the same archetype.
     * Organizes component data in fixed-size chunks (MAX_ENTITIES_IN_CHUNK) for cache efficiency.
     */
    class ArchetypedChunks final
    {
    public:
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
         * @return chunkEntityIndex_t Unique index identifying this entity within the archetype
         *
         * @pre Entity must have all components required by this archetype
         * @post Entity is marked as alive and its component data is stored in the chunk arrays
         */
        chunkEntityIndex_t Create(const PrefabEntity& entity);

        /**
         * @brief Destroys an entity and frees its storage slot.
         *
         * Calls component destructors for all components of the entity and marks the slot as free
         * for future reuse. The memory is not immediately freed but will be reused by subsequent Create calls.
         *
         * @param chunkEntityIndex Index of the entity to destroy
         *
         * @pre Entity must be alive at the given index
         * @post Entity's slot is added to free list and marked as not alive
         */
        void Destroy(chunkEntityIndex_t chunkEntityIndex);

        /**
         * @brief Checks if an entity exists and is alive.
         *
         * @param chunkEntityIndex Index to check
         * @return true if the entity exists and has not been destroyed
         * @return false if the index is out of range or the entity is destroyed
         */
        [[nodiscard]] bool IsAlive(chunkEntityIndex_t chunkEntityIndex) const;

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
         * @brief Gets the highest allocated entity index + 1.
         * Used for iteration bounds checking.
         * @return Number of entity slots allocated (may include dead entities)
         */
        [[nodiscard]] chunkEntityIndex_t getLastChunkEntityIndex() const;

        /**
         * @brief Converts global entity index to chunk index.
         * @param chunkEntityIndex Global entity index
         * @return Index of chunk containing the entity
         */
        [[nodiscard]] static std::size_t getChunkByEntityIndex(chunkEntityIndex_t chunkEntityIndex);

        /**
        * @brief Converts global entity index to position within chunk.
        * @param chunkEntityIndex Global entity index
        * @return Local entity index (0 to MAX_ENTITIES_IN_CHUNK-1)
        */
        [[nodiscard]] static std::size_t getLocalEntityIndex(chunkEntityIndex_t chunkEntityIndex);

        /**
         * @brief Gets the archetype definition for stored entities.
         * @return Const reference to archetype
         */
        [[nodiscard]] const Archetype& archetype() const;

    private:
        Archetype m_archetype;                                                  ///< Archetype definition
        using componentChunks_t = std::vector<std::unique_ptr<byte[]>>;
        std::vector<componentChunks_t> m_chunksByComponentId {};                ///< Per-component: vector of chunk pointers

        std::vector<bool> m_isInWorld;                                          ///< Alive status for each entity slot
        chunkEntityIndex_t m_lastChunkEntityIndex = 0;                          ///< Next free slot index (if no freelist entries)
        std::queue<chunkEntityIndex_t> m_freeChunkEntityIndex;                  ///< Reusable entity slots from destroyed entities
    };

    // =========================================== EntitiesArchetypeStorage ===========================================

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
            using iterator_category = std::forward_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;

            static inline const Archetype ITER_ARCHETYPE = Archetype::GetArchetype<ComponentCls...>();

            constexpr iterator() = default;

            /**
             * @brief Constructs iterator at start or end position.
             * @param storage Storage to iterate over
             */
            explicit iterator(EntitiesArchetypeStorage* storage);

            /**
             * @brief Dereferences iterator.
             * Returns tuple of component references or location based on ValueType.
             * @return Current value (const reference)
             */
            value_type operator*() const;

            /**
             * @brief Access member of current value.
             * @return Pointer to current value
             */
            value_type operator->() const;

            /**
             * @brief Pre-increment: advance to next valid entity.
             * @return Reference to this iterator
             */
            iterator& operator++();

            /**
             * @brief Post-increment: advance to next valid entity.
             * @return Iterator before increment
             */
            iterator operator++(int);

            /**
             * @brief Equality comparison.
             * @param other Iterator to compare with
             * @return true if iterators point to same entity or both at end
             */
            bool operator==(const iterator& other) const;

            /**
             * @brief Inequality comparison.
             * @param other Iterator to compare with
             * @return true if iterators differ
             */
            bool operator!=(const iterator& other) const;

            /**
             * @brief Gets begin iterator for this iterator's storage.
             * @return New iterator at first valid entity
             */
            iterator begin() const;

            /**
             * @brief Gets end iterator for this iterator's storage.
             * @return End iterator
             */
            iterator end() const;

        private:
            void advance();                                         ///< Move to next entity and validate

            EntitiesArchetypeStorage* m_storage = nullptr;          ///< Storage being iterated
            std::size_t m_currentArchetypeIndex = 0;                ///< Current archetype index in m_archetypedChunks
            chunkEntityIndex_t m_chunkEntityIndex {};               ///< Current entity location
            std::vector<ArchetypedChunks*> m_archetypedChunks;
        };

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
         * @brief Gets begin iterator for entities with specified components.
         * @tparam ComponentCls Required component types
         * @return Iterator at first matching entity
         */
        template<IsComponent... ComponentCls>
        [[nodiscard]] auto begin();

        /**
         * @brief Gets end iterator for entities with specified components.
         * @tparam ComponentCls Required component types
         * @return End iterator
         */
        template<IsComponent... ComponentCls>
        [[nodiscard]] auto end() const;

        // ========================================= EntitiesArchetypeStorage =========================================

        /**
         * @brief Creates a new entity from prefab data.
         * Automatically determines or finds appropriate archetype based on prefab's components.
         * @param prefabEntity Prefab containing component data
         * @return Location descriptor for created entity
         */
        ArchetypedChunkEntityLocation Create(const PrefabEntity& prefabEntity);

        /**
         * @brief Destroys entity at given location.
         * Calls destructors and marks slot for reuse.
         * @param entityLocation Location of entity to destroy
         */
        void Destroy(const ArchetypedChunkEntityLocation& entityLocation);

        /**
         * @brief Checks if entity exists and is alive.
         * @param entityLocation Location of entity to check
         * @return true if entity exists
         */
        [[nodiscard]] bool IsAlive(const ArchetypedChunkEntityLocation& entityLocation) const;

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

    private:
        std::unordered_map<archetypeHash_t, archetypeIndex_t> m_archetypeIndexByHash;       ///< Hash to archetype index mapping
        std::vector<ArchetypedChunks> m_storageByArchetypeIndex;                            ///< Storage for each archetype
    };

} // namespace ecs
#endif
#include "detail/EntitiesArchetypeStorage.ipp"
