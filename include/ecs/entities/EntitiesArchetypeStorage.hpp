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
            using iterator_category = std::forward_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;

            static inline const Archetype ITER_ARCHETYPE = Archetype::GetArchetype<ComponentCls...>();

            constexpr iterator() = default;

            explicit iterator(EntitiesArchetypeStorage* storage);

            value_type operator*() const;

            value_type operator->() const;

            iterator& operator++();

            iterator operator++(int);

            bool operator==(const iterator& other) const;

            bool operator!=(const iterator& other) const;

        private:
            void advance();

            using archetypedChunksIt_t = ArchetypedChunks::iterator<iter_value_type<ComponentCls...>, ComponentCls...>;
            std::size_t m_archetypedChunksIndex = 0;
            std::vector<archetypedChunksIt_t> m_archetypedChunks;
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
         * @param entityId global entity id
         * @return Location descriptor for created entity
         */
        ArchetypedChunkEntityLocation Create(const PrefabEntity& prefabEntity, entityId_t entityId);

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
        std::unordered_map<archetypeHash_t, archetypeIndex_t> m_archetypeIndexByHash;       ///< Hash to archetype index mapping
        std::vector<ArchetypedChunks> m_storageByArchetypeIndex;                            ///< Storage for each archetype
    };

} // namespace ecs
#endif
#include "detail/EntitiesArchetypeStorage.ipp"
