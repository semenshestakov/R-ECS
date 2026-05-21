#ifndef ARCHETYPE_HPP
#define ARCHETYPE_HPP
#include <vector>
#include "ecs/utils/ComponentUtils.hpp"
#include "collections/BitSet.hpp"


namespace ecs
{

    using archetypeHash_t = std::size_t;
    using archetypeIndex_t = std::uint16_t;
    
    /**
     * @brief Defines a composition of component types.
     * Archetype represents a unique combination of components and serves as a template
     * for organizing entities with identical component sets.
     */
    struct Archetype final : collection::BitSet
    {

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


        [[nodiscard]] archetypeHash_t hash() const;

        void updateHash();

    private:
        archetypeHash_t m_hash {};                            ///< Hash value uniquely identifying this component combination

    };
}

#endif
#include "detail/Archetype.ipp"