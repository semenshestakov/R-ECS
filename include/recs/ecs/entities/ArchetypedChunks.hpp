#ifndef ARCHETYPED_CHUNKS_HPP
#define ARCHETYPED_CHUNKS_HPP
#include <bit>
#include <tuple>
#include <type_traits>
#include "../components/Utils.hpp"
#include "Archetype.hpp"
#include "Entity.hpp"
#include "Utils.hpp"
#include "common_recs/utils/ClassUtils.hpp"
#include "ecs/entities/PrefabEntity.hpp"


namespace ecs
{

    /**
     * @brief Value type returned by iterator when components are requested.
     * Tuple of references to requested components.
     */
    template<typename... Args>
    using iter_value_type = std::conditional_t<
        (sizeof...(Args) > 0),
        std::tuple<Args&...>,
        chunkEntityIndex_t
    >;


    /**
     * @brief Optional-component marker for views.
     *
     * A view argument written as `Component*` is optional: it does not take part in
     * the archetype filter, and per entity it yields a pointer to the component when
     * the entity's archetype owns it, or `nullptr` otherwise (the @ref
     * ArchetypedChunks::TryGetComponent semantics). A plain `Component` stays required
     * and is yielded by reference.
     */
    template<typename Arg> inline constexpr bool is_optional_component_v = std::is_pointer_v<Arg>;

    /// @brief The underlying component type of a view argument (`Component*` → `Component`).
    template<typename Arg> using component_bare_t = std::remove_pointer_t<Arg>;

    /**
     * @brief Element produced for a single view argument: `Component*` for optional,
     * `Component&` for required.
     */
    template<typename Arg>
    using view_element_t = std::conditional_t<is_optional_component_v<Arg>, Arg, Arg&>;

    /**
     * @brief Const counterpart of @ref view_element_t: `const Component*` for optional,
     * `const Component&` for required.
     */
    template<typename Arg>
    using view_const_element_t = std::conditional_t<
        is_optional_component_v<Arg>,
        const component_bare_t<Arg>*,
        const component_bare_t<Arg>&
    >;

    /**
     * @brief Value type yielded per entity for a (non-Entity) view argument pack.
     *
     * Generalizes @ref iter_value_type to optional components: each argument maps to
     * @ref view_element_t. An empty pack still yields the packed chunk-entity index.
     */
    template<typename... Args>
    using view_value_type = std::conditional_t<
        (sizeof...(Args) > 0),
        std::tuple<view_element_t<Args>...>,
        chunkEntityIndex_t
    >;


    /**
     * @brief Detects whether an iterator value type carries the owning Entity.
     *
     * True only for tuples whose first element is exactly Entity (by value),
     * which is how the Entity-aware views (`view<Entity, Components...>`) request
     * the entity handle alongside its component references. Component-only value
     * types store references (`Component&`), so they never match.
     */
    template<typename T> struct is_entity_value_type : std::false_type {};
    template<typename... Args> struct is_entity_value_type<std::tuple<Entity, Args...>> : std::true_type {};
    template<typename T> inline constexpr bool is_entity_value_type_v = is_entity_value_type<T>::value;


    /**
     * @brief Maps a view argument pack to its iterator instantiation.
     *
     * The pack passed to `view<Args...>()` may optionally start with `Entity`. When
     * it does, the entity is stripped from the component list (entities are not
     * components and must not enter the archetype) and prepended to the value type.
     * The remaining arguments keep their @ref view_element_t mapping, so optional
     * `Component*` arguments are preserved as pointer elements.
     *
     * @tparam IterT  Iterator class template parameterized as `IterT<ValueType, Components...>`.
     * @tparam Args   View argument pack, optionally led by `Entity`.
     */
    template<template<typename, typename...> class IterT, typename... Args>
    struct view_iterator
    {
        using type = IterT<view_value_type<Args...>, Args...>;
    };

    template<template<typename, typename...> class IterT, typename... ComponentCls>
    struct view_iterator<IterT, Entity, ComponentCls...>
    {
        using type = IterT<std::tuple<Entity, view_element_t<ComponentCls>...>, ComponentCls...>;
    };

    template<template<typename, typename...> class IterT, typename... Args>
    using view_iterator_t = typename view_iterator<IterT, Args...>::type;


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

            /**
             * @brief Resolves the base pointer of one component array in the current chunk.
             *
             * Required components use the unchecked accessor (the archetype is known to
             * contain them). Optional `Component*` arguments use the checked accessor, so a
             * chunk whose archetype lacks the component yields a null base — every entity in
             * that chunk then reports the component as absent.
             *
             * @tparam Comp View argument (`Component` or `Component*`).
             * @param chunkBaseIndex Global index of the chunk's first slot.
             * @return Base pointer to the component array, or nullptr for an absent optional.
             */
            template<typename Comp>
            [[nodiscard]] component_bare_t<Comp>* componentBase(const chunkEntityIndex_t chunkBaseIndex) const
            {
                using Bare = component_bare_t<Comp>;
                const componentId_t componentId = ComponentRegistrator::GetComponentId<Bare>();

                if constexpr (is_optional_component_v<Comp>)
                    return std::bit_cast<Bare*>(m_archetypedChunks->GetComponentData(chunkBaseIndex, componentId));
                else
                    return std::bit_cast<Bare*>(m_archetypedChunks->GetComponentDataUnchecked(chunkBaseIndex, componentId));
            }

            /**
             * @brief Produces the value yielded for one component at the current entity.
             *
             * @tparam Comp View argument (`Component` or `Component*`).
             * @return `Component&` for a required argument; a `Component*` (possibly nullptr)
             *         for an optional one.
             */
            template<typename Comp>
            [[nodiscard]] view_element_t<Comp> elementOf() const
            {
                using Bare = component_bare_t<Comp>;
                Bare* base = std::get<Bare*>(m_componentArrays);

                if constexpr (is_optional_component_v<Comp>)
                    return base ? base + m_entityIndex : nullptr;
                else
                    return base[m_entityIndex];
            }

            chunkEntityIndex_t m_chunkIndex = INVALID_CHUNK_ENTITY_INDEX;       ///< Current chunk index.
            chunkEntityIndex_t m_entityIndex = INVALID_CHUNK_ENTITY_INDEX;      ///< Current entity index within the chunk.
            std::tuple<component_bare_t<ComponentCls>*...> m_componentArrays;   ///< Component array base pointers for the current chunk.
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
         * @brief Destroys all components of every live entity still stored here.
         *
         * Iterates the live range of each chunk and calls the destructor of each
         * component in place. No swap-remove relocation is performed, so this is safe
         * to run at teardown regardless of external location bookkeeping. Moved-from
         * source slots left behind by prior swap-remove operations lie outside the live
         * range and are intentionally not destructed.
         */
        ~ArchetypedChunks();

        /**
         * @brief Creates a new entity in this archetype's storage.
         *
         * Allocates a new entity slot, either reusing a freed index or expanding the storage.
         * Copies component data from the prefab entity into the appropriate chunk locations.
         *
         * @param entity PrefabEntity containing all component data to initialize the new entity
         * @param entityHandle Entity handle (id + version)
         * @return chunkEntityIndex_t Unique index identifying this entity within the archetype
         *
         * @pre Entity must have all components required by this archetype
         * @post Entity is marked as alive and its component data is stored in the chunk arrays
         */
        template<PrefabEntityRef PrefabRef>
        chunkEntityIndex_t Create(PrefabRef&& entity, Entity entityHandle);

        /**
         * @brief Reserves a new entity slot without constructing component data.
         *
         * Performs the same slot bookkeeping as Create (reusing a freed chunk index or
         * expanding storage and ensuring backing chunk memory exists for every component),
         * but leaves the component memory raw/uninitialized so the caller can construct or
         * migrate component data into it.
         *
         * @param entityHandle Entity (id + version) stored in the reserved slot
         * @return chunkEntityIndex_t Unique index identifying the reserved slot
         */
        [[nodiscard]] chunkEntityIndex_t AllocateRawSlot(Entity entityHandle);

        /**
         * @brief Destroys an entity and frees its storage slot.
         *
         * Calls component destructors for all components of the entity and marks the slot as free
         * for future reuse. The memory is not immediately freed but will be reused by subsequent Create calls.
         *
         * @param chunkEntityIndex Index of the entity to destroy
         * @return Entity of the swap-removed entity, or invalid Entity if none
         *
         * @pre Entity must be alive at the given index
         * @post Entity's slot is added to free list and marked as not alive
         */
        [[nodiscard]] Entity Destroy(chunkEntityIndex_t chunkEntityIndex);

    private:
        /**
         * @brief Retrieves component data without checking archetype membership.
         *
         * Same address computation as GetComponentData but omits the m_archetype.test()
         * membership check. Intended for hot paths where the archetype is already known to
         * contain the component (asserting accessors, view iteration over a matched archetype).
         *
         * @param chunkEntityIndex Index of the entity
         * @param componentId ID of the component to retrieve
         * @return byte* Pointer to component data
         *
         * @pre m_archetype.test(componentId) == true
         */
        [[nodiscard]] byte* GetComponentDataUnchecked(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId);
        [[nodiscard]] const byte* GetComponentDataUnchecked(chunkEntityIndex_t chunkEntityIndex, componentId_t componentId) const;

    public:
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
         * @brief Number of chunks currently backing this archetype.
         *
         * Chunks are addressable by index in the half-open range [0, chunkCount());
         * combined with aliveInChunk() this exposes the chunk layout so external
         * code (e.g. a parallel scheduler) can partition iteration per chunk.
         *
         * @return Chunk count.
         */
        [[nodiscard]] chunkEntityIndex_t chunkCount() const;

        /**
         * @brief Number of alive entities packed in a given chunk.
         *
         * Alive entities are stored densely in [0, aliveInChunk(chunkIndex)) inside
         * the chunk, so the count doubles as the chunk's local iteration bound.
         *
         * @param chunkIndex Chunk to query (must be < chunkCount()).
         * @return Alive entity count in the chunk.
         */
        [[nodiscard]] chunkEntityIndex_t aliveInChunk(chunkEntityIndex_t chunkIndex) const;

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

    DEEP_TEST_PRIVATE_ACCESS:
        using componentChunks_t = std::vector<std::unique_ptr<byte[]>>;                             ///< Collection of memory chunks storing one component type.

        Archetype m_archetype;                                                                      ///< Archetype shared by all entities stored in this container.
        std::vector<componentChunks_t> m_chunksByComponentId {};                                    ///< Indexed by component ID. Each entry stores memory chunks for that component.
        std::vector<chunkEntityIndex_t> m_chunksEntityCount {};                                     ///< Number of alive entities stored in each chunk.
        collections::BitSet m_hasFreeEntityInChunk;                                                 ///< Tracks chunks that still have free capacity for new entities.

        /// @brief Maps [chunkIndex][localEntityIndex] to Entity (id + version)
        /// Used during entity destruction to retrieve the Entity being swapped.
        std::vector<std::array<Entity, MAX_ENTITIES_IN_CHUNK>> m_localIndexToEntityId;
    };

    // ============================================== ChunkView ===============================================

    /**
     * @brief Lightweight, self-contained view over the alive entities of a single chunk.
     *
     * A ChunkView captures the component array base pointers of one chunk plus its alive
     * count, so it can be copied freely and iterated per entity without touching the owning
     * storage again. It is the unit of work a parallel scheduler hands to a worker: chunks
     * are independent, fixed-capacity and densely packed, which makes them the natural grain
     * for data-parallel iteration even though the cross-chunk iterators are only forward.
     *
     * Iterating a ChunkView yields the same value_type as the entity iterators
     * (a tuple of component references, or the chunk-entity index when no components were
     * requested).
     *
     * @tparam ComponentCls Components exposed through iteration.
     */
    template<IsComponent... ComponentCls>
    class ChunkView
    {
    public:
        using value_type = iter_value_type<ComponentCls...>;        ///< Value produced per entity.

        /**
         * @brief Forward iterator over the alive entities of the chunk.
         */
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = iter_value_type<ComponentCls...>;
            using difference_type = std::ptrdiff_t;

            constexpr iterator() = default;
            iterator(const ChunkView* view, chunkEntityIndex_t index) : m_view(view), m_index(index) {}

            value_type operator*() const
            {
                if constexpr (sizeof...(ComponentCls) == 0)
                    return m_index | m_view->m_chunkIndex;
                else
                    return value_type{std::get<ComponentCls*>(m_view->m_componentArrays)[m_index]...};
            }

            iterator& operator++() { ++m_index; return *this; }
            iterator operator++(int) { iterator copy = *this; ++m_index; return copy; }

            bool operator==(const iterator& other) const { return m_index == other.m_index; }
            bool operator!=(const iterator& other) const { return m_index != other.m_index; }

        private:
            const ChunkView* m_view = nullptr;
            chunkEntityIndex_t m_index = 0;
        };

        /**
         * @brief Constructs a view over a single chunk of an ArchetypedChunks storage.
         *
         * @param chunks Owning chunk storage.
         * @param chunkIndex Chunk to view (must be < chunks->chunkCount()).
         */
        ChunkView(ArchetypedChunks* chunks, chunkEntityIndex_t chunkIndex);

        [[nodiscard]] chunkEntityIndex_t size() const { return m_count; }
        [[nodiscard]] bool empty() const { return m_count == 0; }

        [[nodiscard]] iterator begin() const { return iterator(this, 0); }
        [[nodiscard]] iterator end() const { return iterator(this, m_count); }

    private:
        std::tuple<ComponentCls*...> m_componentArrays {};                 ///< Base pointers of each component array in the chunk.
        chunkEntityIndex_t m_count = 0;                                    ///< Alive entity count in the chunk.
        chunkEntityIndex_t m_chunkIndex = INVALID_CHUNK_ENTITY_INDEX;      ///< Index of the viewed chunk.
    };

} // namespace ecs
#endif
#include "detail/ArchetypedChunks.ipp"