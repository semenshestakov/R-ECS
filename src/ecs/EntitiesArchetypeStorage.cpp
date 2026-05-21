#include <cassert>
#include "ecs/entities/EntitiesArchetypeStorage.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ecs/components/ComponentRegistrator.hpp"

// =========================================== ArchetypedChunkEntityLocation ===========================================

bool ecs::ArchetypedChunkEntityLocation::operator==(const ArchetypedChunkEntityLocation& other) const
{
    return archetypeIndex == other.archetypeIndex && chunkEntityIndex == other.chunkEntityIndex;
}

bool ecs::ArchetypedChunkEntityLocation::operator!=(const ArchetypedChunkEntityLocation& other) const
{
    return !this->operator==(other);
}

// ================================================= ArchetypedChunks =================================================


ecs::ArchetypedChunks::ArchetypedChunks(Archetype a_archetype) :
    m_archetype(std::move(a_archetype))
{
    assert(!m_archetype.empty());
    m_isInWorld.resize(1);
    m_chunksByComponentId.resize(m_archetype.max() + 1);
}

ecs::chunkEntityIndex_t ecs::ArchetypedChunks::Create(const PrefabEntity& entity)
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

void ecs::ArchetypedChunks::Destroy(const chunkEntityIndex_t chunkEntityIndex)
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

bool ecs::ArchetypedChunks::IsAlive(const chunkEntityIndex_t chunkEntityIndex) const
{
    if (m_isInWorld.size() <= chunkEntityIndex)
        return false;

    return m_isInWorld[chunkEntityIndex];
}

ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId)
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

    return &chunk[componentSize * localEntityIndex];
}

const ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId) const
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

    return &chunk[componentSize * localEntityIndex];
}

ecs::chunkEntityIndex_t ecs::ArchetypedChunks::getLastChunkEntityIndex() const
{
    return m_lastChunkEntityIndex;
}

/* static */ std::size_t ecs::ArchetypedChunks::getChunkByEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex / MAX_ENTITIES_IN_CHUNK;
}

/* static */ std::size_t ecs::ArchetypedChunks::getLocalEntityIndex(const chunkEntityIndex_t chunkEntityIndex)
{
    return chunkEntityIndex & MAX_ENTITIES_IN_CHUNK_MASK;
}

const ecs::Archetype& ecs::ArchetypedChunks::archetype() const
{
    return m_archetype;
}

// ============================================= EntitiesArchetypeStorage =============================================

ecs::ArchetypedChunkEntityLocation ecs::EntitiesArchetypeStorage::Create(const PrefabEntity& prefabEntity)
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

void ecs::EntitiesArchetypeStorage::Destroy(const ArchetypedChunkEntityLocation& entityLocation)
{
    assert(IsAlive(entityLocation));
    m_storageByArchetypeIndex.at(entityLocation.archetypeIndex).Destroy(entityLocation.chunkEntityIndex);
}

bool ecs::EntitiesArchetypeStorage::IsAlive(const ArchetypedChunkEntityLocation& entityLocation) const
{
    if (m_storageByArchetypeIndex.size() <= entityLocation.archetypeIndex)
        return false;

    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].IsAlive(entityLocation.chunkEntityIndex);
}

ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(
    const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId
    )
{
    if (!IsAlive(entityLocation))
        return nullptr;

    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}

const ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(
    const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId) const
{
    if (!IsAlive(entityLocation))
        return nullptr;

    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}
