#ifndef ENTITIES_ARCHETYPE_STORAGE_HPP
#define ENTITIES_ARCHETYPE_STORAGE_HPP

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Archetype.hpp"
#include "ArchetypedChunks.hpp"
#include "ecs/utils/ComponentUtils.hpp"
#include "ecs/utils/EntitiesUtils.hpp"


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
             * @brief Archetype composed from all requested ComponentCls types.
             *
             * Used to quickly determine whether a storage archetype contains
             * all components required by this iterator.
             */
            static inline const Archetype s_archetype = Archetype::GetArchetype<ComponentCls...>();

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

            using archetypedChunksIt_t = ArchetypedChunks::iterator<iter_value_type<ComponentCls...>, ComponentCls...>;     ///< Iterator type used for traversal inside a single archetype storage.

            archetypedChunksIt_t m_archetypedChunksIt;                          ///< Current iterator within the active ArchetypedChunks instance.
            archetypeIndex_t m_archetypeIndex = INVALID_ARCHETYPE_INDEX;        ///< Index of currently traversed archetype.
            EntitiesArchetypeStorage* m_storage = nullptr;                      ///< Storage being iterated.
        };

        /**
         * @brief Creates iterator over entities containing all specified components.
         *
         * Iteration spans all archetypes whose component set is a superset of
         * ComponentCls....
         *
         * @tparam ComponentCls Required component types.
         * @return Iterator positioned at the first matching entity.
         */
        template<IsComponent... ComponentCls>
        [[nodiscard]] auto begin();

        /**
         * @brief Returns iterator representing end of traversal.
         *
         * @tparam ComponentCls Component filter type list.
         * @return End iterator.
         */
        template<IsComponent... ComponentCls>
        [[nodiscard]] auto end() const;

        // ========================================= EntitiesArchetypeStorage =========================================

        /**
         * @brief Creates a new entity from prefab data.
         * Automatically determines or finds appropriate archetype based on prefab's components.
         * @param prefabEntity Prefab containing component data
         * @param entityId global entity id
         * @return Location descriptor for created entity
         */
        template<PrefabEntityRef PrefabRef>
        ArchetypedChunkEntityLocation Create(PrefabRef&& prefabEntity, entityId_t entityId);

        /**
         * @brief Destroys entity at given location.
         * Calls destructors and marks slot for reuse.
         * @param entityLocation Location of entity to destroy
         */
        entityId_t Destroy(const ArchetypedChunkEntityLocation& entityLocation);

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
