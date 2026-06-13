#pragma once
#include "../Archetype.hpp"
#include "ecs/components/ComponentRegistrator.hpp"


inline ecs::chunkEntityIndex_t ecs::getChunkByEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex / MAX_ENTITIES_IN_CHUNK;
}

inline ecs::chunkEntityIndex_t ecs::getLocalEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex & MAX_ENTITIES_IN_CHUNK_MASK;
}

// ===================================================== Archetype =====================================================

inline ecs::archetypeHash_t ecs::Archetype::hash() const
{
    return m_hash;
}

inline void ecs::Archetype::updateHash()
{
    m_hash = GetArchetypeHash(*this);
}

/* static */ constexpr ecs::archetypeHash_t ecs::Archetype::GetArchetypeHash(const collections::BitSet& bits)
{
    archetypeHash_t result = 0x9e3779b9;
    for(const auto bitId: bits.data())
    {
        result ^= bitId + 0x9e3779b9 + (result << 6) + (result >> 2);
    }
    return result;
}

template<ecs::IsComponent... ComponentCls>
/* static */ const ecs::Archetype& ecs::Archetype::GetArchetype()
{
    static const Archetype s_archetype = []() {
        Archetype archetype;
        if(sizeof...(ComponentCls) > 0)
        {
            (archetype.set(ComponentRegistrator::GetComponentId<ComponentCls>()), ...);
            archetype.m_hash = Archetype::GetArchetypeHash(archetype);
        }
        return archetype;
    }();

    return s_archetype;
}

// =========================================== ArchetypedChunkEntityLocation ===========================================

inline bool ecs::ArchetypedChunkEntityLocation::operator==(const ArchetypedChunkEntityLocation& other) const
{
    return archetypeIndex == other.archetypeIndex && chunkEntityIndex == other.chunkEntityIndex;
}

inline bool ecs::ArchetypedChunkEntityLocation::operator!=(const ArchetypedChunkEntityLocation& other) const
{
    return !this->operator==(other);
}
