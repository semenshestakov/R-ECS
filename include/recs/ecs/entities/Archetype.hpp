#ifndef ARCHETYPE_HPP
#define ARCHETYPE_HPP
#include "../components/Utils.hpp"
#include "collections/BitSet.hpp"


namespace ecs
{

    using archetypeHash_t = std::size_t;
    using archetypeIndex_t = std::uint16_t;
    using chunkEntityIndex_t = std::uint32_t;

    constexpr archetypeIndex_t INVALID_ARCHETYPE_INDEX = ~static_cast<archetypeIndex_t>(0u);

    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK_BITS = 10;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK = 1 << MAX_ENTITIES_IN_CHUNK_BITS;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNK_MASK = MAX_ENTITIES_IN_CHUNK - 1;
    constexpr chunkEntityIndex_t MAX_ENTITIES_IN_CHUNKS = ~0u;
    constexpr chunkEntityIndex_t INVALID_CHUNK_ENTITY_INDEX = ~0u;

    /**
     * @brief Converts global entity index to chunk index.
     * @param chunkEntityIndex Global entity index
     * @return Index of chunk containing the entity
     */
    [[nodiscard]] chunkEntityIndex_t getChunkByEntityIndex(chunkEntityIndex_t chunkEntityIndex);

    /**
     * @brief Converts global entity index to position within chunk.
     * @param chunkEntityIndex Global entity index
     * @return Local entity index (0 to MAX_ENTITIES_IN_CHUNK-1)
     */
    [[nodiscard]] chunkEntityIndex_t getLocalEntityIndex(chunkEntityIndex_t chunkEntityIndex);

    /**
     * @brief Defines a composition of component types.
     * Archetype represents a unique combination of components and serves as a template
     * for organizing entities with identical component sets.
     */
    struct Archetype final : collections::BitSet
    {

        /**
         * @brief Returns the hash value of this archetype
         * @return Hash value
         */
        [[nodiscard]] archetypeHash_t hash() const;

        /**
         * @brief Updates the hash value based on current bitset state
         */
        void updateHash();

        /**
         * @brief Computes hash from a range of component IDs using a hash combining algorithm.
         * @param bits set of bit
         * @return Combined hash value for the component ID sequence
         */
        static constexpr archetypeHash_t GetArchetypeHash(const BitSet& bits);

        /**
         * @brief Gets or creates the archetype for specified component types.
         * Creates a static singleton archetype instance for the given component combination.
         * @tparam ComponentCls Component types that form the archetype
         * @return Const reference to the archetype instance
         */
        template<IsComponent... ComponentCls>
        static const Archetype& GetArchetype();

    private:
        archetypeHash_t m_hash {};                            ///< Hash value uniquely identifying this component combination

    };


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

}

#endif
#include "detail/Archetype.ipp"