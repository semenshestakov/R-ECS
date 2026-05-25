#pragma once
#include <cassert>
#include "../EntitiesArchetypeStorage.hpp"
#include "ecs/entities/PrefabEntity.hpp"

// ====================================== EntitiesArchetypeStorage::iterator ======================================


template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::iterator(EntitiesArchetypeStorage* storage) :
    m_storage(storage)
{
    if (m_storage == nullptr)
        return;

    for (auto& chunks : m_storage->m_storageByArchetypeIndex)
    {
        if(chunks.archetype().isSubsetOf(ITER_ARCHETYPE))
            m_archetypedChunks.push_back(&chunks);
    }

    if (m_archetypedChunks.empty())
    {
        m_storage = nullptr; // set end
        return;
    }

    if (!m_archetypedChunks[m_currentArchetypeIndex]->IsAlive(m_chunkEntityIndex))
        advance();
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
typename ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::value_type ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator*() const
{
    if constexpr(std::is_same_v<std::tuple<ComponentCls&...>, value_type>)
    {
        return std::forward_as_tuple(m_archetypedChunks[m_currentArchetypeIndex]->template GetComponent<ComponentCls>(m_chunkEntityIndex)...);
    }
    else if constexpr(std::is_same_v<ArchetypedChunkEntityLocation, value_type>)
    {
        return m_chunkEntityIndex;
    }
    else
    {
        assert(false);
    }
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
typename ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::value_type  ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator->() const
{
    return &this->operator*();
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
    iterator tmp = *this;
    advance();
    return tmp;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator==(const iterator& other) const
{
    if (other.m_storage == nullptr && m_storage == nullptr)
        return true;

    return m_currentArchetypeIndex == other.m_currentArchetypeIndex && m_chunkEntityIndex == other.m_chunkEntityIndex && m_storage == other.m_storage;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator!=(const iterator& other) const
{
    return !(*this == other);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::begin() const
{
    return iterator<ValueType, ComponentCls...>(m_storage);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::end() const
{
    return iterator<ValueType, ComponentCls...>();
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::advance()
{
    const ArchetypedChunks* chunks = m_archetypedChunks[m_currentArchetypeIndex];

    const chunkEntityIndex_t limit = chunks->getLastChunkEntityIndex();
    chunkEntityIndex_t chunkEntityIndex = m_chunkEntityIndex + 1;
    for (; chunkEntityIndex < limit; ++chunkEntityIndex)
    {
        if(chunks->IsAlive(chunkEntityIndex))
        {
            m_chunkEntityIndex = chunkEntityIndex;
            return;
        }
    }

    if (chunkEntityIndex == limit)
    {
        ++m_currentArchetypeIndex;
        if (m_currentArchetypeIndex == m_archetypedChunks.size())
        {
            m_storage = nullptr;
            return;
        }

        m_chunkEntityIndex = 0;
    }

}


// ===================================== EntitiesArchetypeStorage::begin | end =====================================

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

// ======================================= EntitiesArchetypeStorage::methods =======================================

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location)
{
    return std::launder(reinterpret_cast<ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>())));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location) const
{
    return std::launder(reinterpret_cast<const ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>())));
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

// ================================================= ArchetypedChunks =================================================


inline ecs::ArchetypedChunks::ArchetypedChunks(Archetype a_archetype) :
    m_archetype(std::move(a_archetype))
{
    assert(!m_archetype.empty());
    m_isInWorld.resize(1);
    m_chunksByComponentId.resize(m_archetype.max() + 1);
}

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::Create(const PrefabEntity& entity)
{
    // - - - Find Chunk Index, Entity Index - - -
    chunkEntityIndex_t chunkEntityIndex;
    if (m_freeChunkEntityIndex.empty())
    {
        chunkEntityIndex = m_lastChunkEntityIndex++;
        if (m_isInWorld.size() <= chunkEntityIndex)
        {
            m_isInWorld.resize(m_isInWorld.size() * 2);
        }
    }
    else
    {
        chunkEntityIndex = m_freeChunkEntityIndex.front();
        m_freeChunkEntityIndex.pop();
    }
    assert(!m_isInWorld[chunkEntityIndex]);

    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

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

    m_isInWorld[chunkEntityIndex] = true;
    return chunkEntityIndex;
}

inline void ecs::ArchetypedChunks::Destroy(const chunkEntityIndex_t chunkEntityIndex)
{
    assert(m_isInWorld[chunkEntityIndex]);

    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    for (const componentId_t componentId : m_archetype)
    {
        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);

        componentInfo.destructor(
            &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex]
            );
    }

    m_isInWorld[chunkEntityIndex] = false;
    m_freeChunkEntityIndex.emplace(chunkEntityIndex);
}

inline bool ecs::ArchetypedChunks::IsAlive(const chunkEntityIndex_t chunkEntityIndex) const
{
    if (m_isInWorld.size() <= chunkEntityIndex)
        return false;

    return m_isInWorld[chunkEntityIndex];
}

inline ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId)
{
    if (!IsAlive(chunkEntityIndex))
        return nullptr;

    if (componentId >= m_chunksByComponentId.size())
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];
    if (componentChunks.empty())
        return nullptr;

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    if (chunkIndex >= componentChunks.size())
        return nullptr;

    auto& chunk = componentChunks[chunkIndex];
    if (!chunk)
        return nullptr;

    if (localEntityIndex >= MAX_ENTITIES_IN_CHUNK)
        return nullptr;

    return std::launder(&chunk[componentSize * localEntityIndex]);
}

inline const ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId) const
{
    if (!IsAlive(chunkEntityIndex))
        return nullptr;

    if (componentId >= m_chunksByComponentId.size())
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];
    if (componentChunks.empty())
        return nullptr;

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    if (chunkIndex >= componentChunks.size())
        return nullptr;

    auto& chunk = componentChunks[chunkIndex];
    if (!chunk)
        return nullptr;

    if (localEntityIndex >= MAX_ENTITIES_IN_CHUNK)
        return nullptr;

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

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::getLastChunkEntityIndex() const
{
    return m_lastChunkEntityIndex;
}

/* static */
inline std::size_t ecs::ArchetypedChunks::getChunkByEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex / MAX_ENTITIES_IN_CHUNK;
}

/* static */
inline std::size_t ecs::ArchetypedChunks::getLocalEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex & MAX_ENTITIES_IN_CHUNK_MASK;
}

inline const ecs::Archetype& ecs::ArchetypedChunks::archetype() const
{
    return m_archetype;
}

// ============================================= EntitiesArchetypeStorage =============================================

inline ecs::ArchetypedChunkEntityLocation ecs::EntitiesArchetypeStorage::Create(const PrefabEntity& prefabEntity)
{
    // - - - Calculate Archetype - - -

    const PrefabEntity::componentsData_t& componentsData = prefabEntity.GetComponentsData();

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
        .chunkEntityIndex=m_storageByArchetypeIndex[archetypeIndex].Create(prefabEntity)
    };
}

inline void ecs::EntitiesArchetypeStorage::Destroy(const ArchetypedChunkEntityLocation& entityLocation)
{
    assert(IsAlive(entityLocation));
    m_storageByArchetypeIndex.at(entityLocation.archetypeIndex).Destroy(entityLocation.chunkEntityIndex);
}

inline bool ecs::EntitiesArchetypeStorage::IsAlive(const ArchetypedChunkEntityLocation& entityLocation) const
{
    if (m_storageByArchetypeIndex.size() <= entityLocation.archetypeIndex)
        return false;

    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].IsAlive(entityLocation.chunkEntityIndex);
}

inline ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(
    const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId
    )
{
    if (!IsAlive(entityLocation))
        return nullptr;

    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}

inline const ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(
    const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId) const
{
    if (!IsAlive(entityLocation))
        return nullptr;

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
