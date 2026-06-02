#pragma once
#include <cassert>
#include "../ArchetypedChunks.hpp"
#include "ecs/entities/PrefabEntity.hpp"


// ============================================ ArchetypedChunks::iterator =============================================

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::iterator(ArchetypedChunks* archetypedChunks) :
    m_archetypedChunks(archetypedChunks)
{
    assert(m_archetypedChunks != nullptr);

    if (m_archetypedChunks->m_chunksEntityCount.empty() || m_archetypedChunks->m_chunksEntityCount[0] == 0)
        advance();
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::iterator(ArchetypedChunks* archetypedChunks, bool isEnd) :
    m_archetypedChunks(archetypedChunks)
{
    assert(m_archetypedChunks != nullptr);
    m_chunkEntityIndex = m_archetypedChunks->m_chunksEntityCount.size();
}

template<typename ValueType, ecs::IsComponent... ComponentCls> ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::value_type
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator*() const
{
    assert(m_archetypedChunks);

    if constexpr (sizeof...(ComponentCls) == 0)
        return m_chunkEntityIndex;
    else
        return value_type{ m_archetypedChunks->template GetComponent<ComponentCls>(m_chunkEntityIndex)... };
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::value_type ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator->() const
{
    return **this;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>& ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator++()
{
    advance();
    return *this;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...> ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator++(int)
{
    iterator copy = *this;
    ++(*this);
    return copy;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator==(const iterator& other) const
{
    return m_chunkEntityIndex == other.m_chunkEntityIndex;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator!=(const iterator& other) const
{
    return !(*this == other);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator bool() const
{
    return getChunkByEntityIndex(m_chunkEntityIndex) < m_archetypedChunks->m_chunksEntityCount.size();
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::advance()
{
    assert(m_archetypedChunks != nullptr);
    ++m_chunkEntityIndex;

    while (true)
    {
        const std::size_t chunkIndex = getChunkByEntityIndex(m_chunkEntityIndex);

        if (chunkIndex >= m_archetypedChunks->m_chunksEntityCount.size())
            return;

        if (const std::size_t localIndex = getLocalEntityIndex(m_chunkEntityIndex);
           localIndex < m_archetypedChunks->m_chunksEntityCount[chunkIndex])
            return;

        m_chunkEntityIndex = (chunkIndex + 1) << MAX_ENTITIES_IN_CHUNK_BITS;
    }
}

// ================================================= ArchetypedChunks ==================================================

inline ecs::ArchetypedChunks::ArchetypedChunks(Archetype a_archetype) :
    m_archetype(std::move(a_archetype))
{
    assert(!m_archetype.empty());
    m_localIndexToEntityId.resize(1);
    m_chunksByComponentId.resize(m_archetype.max() + 1);

    m_hasFreeEntityInChunk.set(0);
}

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::Create(const PrefabEntity& entity, const entityId_t entityId)
{
    // - - - Find Chunk Index, Entity Index - - -
    chunkEntityIndex_t chunkIndex;
    chunkEntityIndex_t localEntityIndex;

    if (const std::size_t minChunkIndexHasFreeEntities = m_hasFreeEntityInChunk.min(); minChunkIndexHasFreeEntities == decltype(m_hasFreeEntityInChunk)::INVALID_INDEX)
    {
        chunkIndex = m_hasFreeEntityInChunk.size();
        m_hasFreeEntityInChunk.set(chunkIndex);
        localEntityIndex = m_chunksEntityCount[chunkIndex] = 1;

        m_localIndexToEntityId.resize(chunkIndex * 2);
    }
    else
    {
        localEntityIndex = m_chunksEntityCount[minChunkIndexHasFreeEntities]++;
        chunkIndex = minChunkIndexHasFreeEntities;

        if (m_chunksEntityCount[minChunkIndexHasFreeEntities] == MAX_ENTITIES_IN_CHUNK)
            m_hasFreeEntityInChunk.reset(minChunkIndexHasFreeEntities);
    }
    assert(m_localIndexToEntityId[chunkIndex][localEntityIndex] == INVALID_ENTITY_ID);

    // - - - Fill Data For Components - - -

    const PrefabEntity::componentsData_t& componentsData = entity.GetComponentsData();
    for (const componentId_t componentId : m_archetype)
    {
        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);
        componentChunks_t& componentChunks = m_chunksByComponentId[componentId];

        if (componentChunks.size() <= chunkIndex)
        {
            componentChunks.reserve(chunkIndex * 2);
            componentChunks.resize(chunkIndex + 1);
            componentChunks[chunkIndex] = std::make_unique<byte[]>(componentInfo.componentSize * MAX_ENTITIES_IN_CHUNK);
        }

        assert(componentsData[componentId] != nullptr);

        componentInfo.copy(
            &componentChunks[chunkIndex][componentInfo.componentSize * localEntityIndex],
            componentsData[componentId].get()
            );
    }

    m_localIndexToEntityId[chunkIndex][localEntityIndex] = entityId;
    return localEntityIndex + (chunkIndex  << MAX_ENTITIES_IN_CHUNK_BITS);
}

inline ecs::entityId_t ecs::ArchetypedChunks::Destroy(const chunkEntityIndex_t chunkEntityIndex)
{
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    for (const componentId_t componentId : m_archetype)
    {
        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);

        componentInfo.destructor(
            &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex]
            );
    }

    entityId_t migrationEntityId;
    const chunkEntityIndex_t lastChunkEntityIndex = --m_chunksEntityCount[chunkIndex];

    if (lastChunkEntityIndex != localEntityIndex)
    {
        migrationEntityId = m_localIndexToEntityId[chunkIndex][lastChunkEntityIndex];
        m_localIndexToEntityId[chunkIndex][localEntityIndex] = migrationEntityId;
        for (const componentId_t componentId : m_archetype)
        {
            const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);

            componentInfo.move(
                /* to */    &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex],
                /* from */  &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * lastChunkEntityIndex]
                );
        }
    }
    else
    {
        migrationEntityId = INVALID_ENTITY_ID;
    }

    m_localIndexToEntityId[chunkIndex][lastChunkEntityIndex] = INVALID_ENTITY_ID;
    m_hasFreeEntityInChunk.set(chunkIndex);

    return migrationEntityId;
}


inline ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId)
{
    if(!m_archetype.test(componentId))
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    auto& chunk = componentChunks[chunkIndex];
    return std::launder(&chunk[componentSize * localEntityIndex]);
}

inline const ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId) const
{
    if(!m_archetype.test(componentId))
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    auto& chunk = componentChunks[chunkIndex];
    return std::launder(&chunk[componentSize * localEntityIndex]);
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::ArchetypedChunks::TryGetComponent(const chunkEntityIndex_t chunkEntityIndex)
{
    return std::launder(reinterpret_cast<ComponentCls*>(GetComponentData(chunkEntityIndex, ComponentRegistrator::GetСomponentId<ComponentCls>())));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::ArchetypedChunks::TryGetComponent(const chunkEntityIndex_t chunkEntityIndex) const
{
    return std::launder(reinterpret_cast<const ComponentCls*>(GetComponentData(chunkEntityIndex, ComponentRegistrator::GetСomponentId<ComponentCls>())));
}

template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::ArchetypedChunks::GetComponent(const chunkEntityIndex_t chunkEntityIndex)
{
    ComponentCls* componentData = TryGetComponent<ComponentCls>(chunkEntityIndex);
    assert(componentData != nullptr);
    return *componentData;
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::ArchetypedChunks::GetComponent(const chunkEntityIndex_t chunkEntityIndex) const
{
    const ComponentCls* componentData = TryGetComponent<ComponentCls>(chunkEntityIndex);
    assert(componentData != nullptr);
    return *componentData;
}

inline const ecs::Archetype& ecs::ArchetypedChunks::archetype() const
{
    return m_archetype;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
auto ecs::ArchetypedChunks::begin()
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>(this);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
auto ecs::ArchetypedChunks::end()
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>(this, true);
}

