#pragma once
#include "ecs/components/ComponentRegistrator.hpp"


/* static */ constexpr ecs::archetypeHash_t ecs::Archetype::GetArchetypeHash(const collection::BitSet& bits)
{
    archetypeHash_t result = 0x9e3779b9;
    for(const std::size_t bitId: bits)
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
            (archetype.set(ComponentRegistrator::GetСomponentId<ComponentCls>()), ...);
            archetype.m_hash = Archetype::GetArchetypeHash(archetype);
        }
        return archetype;
    }();

    return s_archetype;
}

inline ecs::archetypeHash_t ecs::Archetype::hash() const
{
    return m_hash;
}

inline void ecs::Archetype::updateHash()
{
    m_hash = GetArchetypeHash(*this);
}
