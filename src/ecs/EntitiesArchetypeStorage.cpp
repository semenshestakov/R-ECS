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

std::vector<bool> ecs::Archetype::GetArchetypeMask(const componentId_t* begin, const componentId_t* end)
{
    componentId_t maxId = 0;
    for (const componentId_t* _begin = begin; _begin != end; ++_begin)
    {
        maxId = std::max(maxId, *_begin);
    }
    assert(maxId > 0);

    std::vector<bool> resultMask;
    resultMask.resize(maxId + 1);

    for (const componentId_t* _begin = begin; _begin != end; ++_begin)
    {
        resultMask[*_begin] = true;
    }

    return resultMask;
}

ecs::ArchetypedChunks::ArchetypedChunks(Archetype&& a_archetype) :
    m_archetype(std::move(a_archetype))
{
    assert(!m_archetype.componentsIds.empty());
    m_isInWorld.resize(1);
    m_chunksByComponentId.resize(m_archetype.componentsIds.back() + 1); // sorted componentsIds [min -> max]
}

ecs::chunkEntityIndex_t ecs::ArchetypedChunks::Create(const PrefabEntity& entity)
{
    // - - - Find Chunk Index, Entity Index - - -
    chunkEntityIndex_t chunkEntityIndex;
    if (m_freeChunkEntityIndex.empty())
    {
        chunkEntityIndex = m_lastChunkEntityIndex++;
        m_isInWorld.resize(chunkEntityIndex + 1);
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
    for (const componentId_t componentId : m_archetype.componentsIds)
    {
        const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
        componentChunks_t& componentChunks = m_chunksByComponentId[componentId];

        if (componentChunks.size() <= chunkIndex)
        {
            componentChunks.resize(chunkIndex + 1);
            componentChunks[chunkIndex] = std::make_unique<byte[]>(componentSize * MAX_ENTITIES_IN_CHUNK);
        }

        assert(componentsData[componentId] != nullptr);
        memcpy(
            /*  to  */ &componentChunks[chunkIndex][componentSize * localEntityIndex],
            /* from */ componentsData[componentId].get(),
            /* size */ componentSize
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

    for (const componentId_t componentId : m_archetype.componentsIds)
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

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    return &m_chunksByComponentId[componentId][chunkIndex][componentSize * localEntityIndex];
}

const ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId) const
{
    if (!IsAlive(chunkEntityIndex))
        return nullptr;

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    return &m_chunksByComponentId[componentId][chunkIndex][componentSize * localEntityIndex];
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

    Archetype archetype;
    archetype.componentsIds.reserve(componentsData.size() + 1);

    for (std::size_t componentId = 0; componentId < componentsData.size(); componentId++)
    {
        if (componentsData[componentId] != nullptr)
        {
            archetype.componentsIds.push_back(componentId);
        }
    }

    archetype.hash = Archetype::GetArchetypeHash(
        archetype.componentsIds.data(), archetype.componentsIds.data() + archetype.componentsIds.size()
        );
    archetype.mask = Archetype::GetArchetypeMask(
        archetype.componentsIds.data(), archetype.componentsIds.data() + archetype.componentsIds.size()
        );

    // - - - Find Archetype Index - - -

    archetypeIndex_t archetypeIndex;
    if (const auto it = m_archetypeIndexByHash.find(archetype.hash); it != m_archetypeIndexByHash.end())
    {
        archetypeIndex = it->second;
    }
    else
    {
        // make new index
        assert(m_archetypeIndexByHash.size() == m_storageByArchetypeIndex.size());
        archetypeIndex = m_archetypeIndexByHash.size();

        m_archetypeIndexByHash[archetype.hash] = archetypeIndex;
        m_storageByArchetypeIndex.emplace_back(ArchetypedChunks(std::move(archetype)));
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
