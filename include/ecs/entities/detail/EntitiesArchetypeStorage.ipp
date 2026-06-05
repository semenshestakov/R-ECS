#pragma once
#include <cassert>
#include "ecs/entities/EntitiesArchetypeStorage.hpp"
#include "ecs/entities/PrefabEntity.hpp"

// ======================================== EntitiesArchetypeStorage::iterator =========================================


template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::iterator(EntitiesArchetypeStorage* storage)
{
    assert(storage != nullptr);

    for (auto& chunks : storage->m_storageByArchetypeIndex)
    {
        if(ITER_ARCHETYPE.isSubsetOf(chunks.archetype()))
            m_archetypedChunks.emplace_back(chunks.begin<ValueType, ComponentCls...>());
    }
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
typename ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::value_type ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator*() const
{
    return *m_archetypedChunks[m_archetypedChunksIndex];
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>&
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator++()
{
    advance();
    return *this;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator++(int)
{
    iterator copy = *this;
    advance();
    return copy;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator==(const iterator& other) const
{
    const bool thisEnd =
        m_archetypedChunksIndex >= m_archetypedChunks.size();

    const bool otherEnd =
        other.m_archetypedChunksIndex >= other.m_archetypedChunks.size();

    if (thisEnd && otherEnd)
        return true;

    if (thisEnd != otherEnd)
        return false;

    return m_archetypedChunksIndex == other.m_archetypedChunksIndex &&
           m_archetypedChunks[m_archetypedChunksIndex] ==
           other.m_archetypedChunks[other.m_archetypedChunksIndex];
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator!=(const iterator& other) const
{
    return !(*this == other);
}


template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::advance()
{
    if (m_archetypedChunksIndex >= m_archetypedChunks.size())
        return;

    ++m_archetypedChunks[m_archetypedChunksIndex];

    while (m_archetypedChunksIndex < m_archetypedChunks.size())
    {
        if (m_archetypedChunks[m_archetypedChunksIndex])
            return;

        ++m_archetypedChunksIndex;
    }
}

// ============================================= EntitiesArchetypeStorage ==============================================

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesArchetypeStorage::begin()
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>(this);
}

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesArchetypeStorage::end() const
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>();
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location)
{
    return std::bit_cast<ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location) const
{
    return std::bit_cast<const ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}


inline ecs::ArchetypedChunkEntityLocation ecs::EntitiesArchetypeStorage::Create(const PrefabEntity& prefabEntity, const entityId_t entityId)
{
    // - - - Calculate Archetype - - -
    const Archetype& archetype = prefabEntity.getArchetype();

    // - - - Find Archetype Index - - -

    archetypeIndex_t archetypeIndex;
    if (const auto it = m_archetypeIndexByHash.find(archetype.hash()); it != m_archetypeIndexByHash.end())
    {
        archetypeIndex = it->second;
    }
    else
    {
        // make new index
        assert(m_archetypeIndexByHash.size() == m_storageByArchetypeIndex.size());
        archetypeIndex = m_archetypeIndexByHash.size();

        m_archetypeIndexByHash[archetype.hash()] = archetypeIndex;
        m_storageByArchetypeIndex.emplace_back(archetype);
    }

    return {
        .archetypeIndex=archetypeIndex,
        .chunkEntityIndex=m_storageByArchetypeIndex[archetypeIndex].Create(prefabEntity, entityId)
    };
}

inline ecs::entityId_t ecs::EntitiesArchetypeStorage::Destroy(const ArchetypedChunkEntityLocation& entityLocation)
{
    return m_storageByArchetypeIndex.at(entityLocation.archetypeIndex).Destroy(entityLocation.chunkEntityIndex);
}

inline ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId)
{
    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}

inline const ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId) const
{
    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}

template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::EntitiesArchetypeStorage::GetComponent(const ArchetypedChunkEntityLocation& location)
{
    ComponentCls* componentData = TryGetComponent<ComponentCls>(location);
    assert(componentData != nullptr);
    return *componentData;
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::EntitiesArchetypeStorage::GetComponent(const ArchetypedChunkEntityLocation& location) const
{
    const ComponentCls* componentData = TryGetComponent<ComponentCls>(location);
    assert(componentData != nullptr);
    return *componentData;
}
