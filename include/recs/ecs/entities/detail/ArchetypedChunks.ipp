#pragma once
#include <cassert>
#include "../ArchetypedChunks.hpp"


// ============================================ ArchetypedChunks::iterator =============================================

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::iterator(ArchetypedChunks* archetypedChunks) :
    m_archetypedChunks(archetypedChunks),
    m_chunkIndex(0),
    m_entityIndex(0)
{
    assert(m_archetypedChunks != nullptr);

    if (m_archetypedChunks->m_chunksEntityCount.empty() || m_archetypedChunks->m_chunksEntityCount[0] == 0)
        advance();

    if (*this)
    {
        m_componentArrays = std::tuple{
            std::bit_cast<ComponentCls*>(
                m_archetypedChunks->GetComponentData(0, ComponentRegistrator::GetComponentId<ComponentCls>())
            )...
        };
    }
}

template<typename ValueType, ecs::IsComponent... ComponentCls> ValueType ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator*() const
{
    assert(m_archetypedChunks != nullptr);

    if constexpr (sizeof...(ComponentCls) == 0)
        return m_entityIndex | m_chunkIndex;
    else
        return value_type{
            std::get<ComponentCls*>(m_componentArrays)[m_entityIndex]...
        };
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
    return m_chunkIndex == other.m_chunkIndex && m_entityIndex == other.m_entityIndex;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator!=(const iterator& other) const
{
    return !(*this == other);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::operator bool() const
{
    return m_chunkIndex != INVALID_CHUNK_ENTITY_INDEX;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::ArchetypedChunks::iterator<ValueType, ComponentCls...>::advance()
{
    assert(m_archetypedChunks != nullptr);
    ++m_entityIndex;

    while (m_chunkIndex < m_archetypedChunks->m_chunksEntityCount.size())
    {
        if (m_entityIndex < m_archetypedChunks->m_chunksEntityCount[m_chunkIndex])
            return;

        ++m_chunkIndex;

        if (m_chunkIndex >= m_archetypedChunks->m_chunksEntityCount.size())
        {
            m_chunkIndex = m_entityIndex = INVALID_CHUNK_ENTITY_INDEX;
            return;
        }

        m_componentArrays = std::tuple{
            std::bit_cast<ComponentCls*>(
                m_archetypedChunks->GetComponentData(m_chunkIndex, ComponentRegistrator::GetComponentId<ComponentCls>())
            )...
        };
        m_entityIndex = 0;

        if (m_archetypedChunks->m_chunksEntityCount[m_chunkIndex] > 0)
            return;
    }

}

// ================================================= ArchetypedChunks ==================================================

inline ecs::ArchetypedChunks::ArchetypedChunks(Archetype a_archetype) : m_archetype(std::move(a_archetype))
{
    assert(!m_archetype.empty());
    m_localIndexToEntityId.resize(1);
    m_chunksByComponentId.resize(m_archetype.max() + 1);
    m_chunksEntityCount.resize(1);

    m_hasFreeEntityInChunk.set(0);
}

inline ecs::ArchetypedChunks::~ArchetypedChunks()
{
    for (std::size_t chunkIndex = 0; chunkIndex < m_chunksEntityCount.size(); ++chunkIndex)
    {
        const chunkEntityIndex_t aliveCount = m_chunksEntityCount[chunkIndex];

        for (chunkEntityIndex_t localEntityIndex = 0; localEntityIndex < aliveCount; ++localEntityIndex)
        {
            for (const componentId_t componentId : m_archetype)
            {
                const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);
                componentInfo.destructor(
                    &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex]);
            }
        }
    }
}


inline ecs::entityId_t ecs::ArchetypedChunks::Destroy(const chunkEntityIndex_t chunkEntityIndex)
{
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    for (const componentId_t componentId : m_archetype)
    {
        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);

        componentInfo.destructor(
                &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex]);
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
                    /* to */ &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex],
                    /* from */ &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * lastChunkEntityIndex]
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
    if (!m_archetype.test(componentId))
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    auto& chunk = componentChunks[chunkIndex];
    return &chunk[componentSize * localEntityIndex];
}


inline const ecs::byte* ecs::ArchetypedChunks::GetComponentData(const chunkEntityIndex_t chunkEntityIndex, const componentId_t componentId) const
{
    if (!m_archetype.test(componentId))
        return nullptr;

    const auto& componentChunks = m_chunksByComponentId[componentId];

    const bufferSize_t componentSize = ComponentRegistrator::GetInfo(componentId).componentSize;
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    auto& chunk = componentChunks[chunkIndex];
    return &chunk[componentSize * localEntityIndex];
}

template <ecs::IsComponent ComponentCls>
ComponentCls* ecs::ArchetypedChunks::TryGetComponent(const chunkEntityIndex_t chunkEntityIndex)
{
    return std::bit_cast<ComponentCls*>(
            GetComponentData(chunkEntityIndex, ComponentRegistrator::GetComponentId<ComponentCls>()));
}

template <ecs::IsComponent ComponentCls>
const ComponentCls* ecs::ArchetypedChunks::TryGetComponent(const chunkEntityIndex_t chunkEntityIndex) const
{
    return std::bit_cast<const ComponentCls*>(GetComponentData(chunkEntityIndex, ComponentRegistrator::GetComponentId<ComponentCls>()));
}

template <ecs::IsComponent ComponentCls>
ComponentCls& ecs::ArchetypedChunks::GetComponent(const chunkEntityIndex_t chunkEntityIndex)
{
    ComponentCls* componentData = TryGetComponent<ComponentCls>(chunkEntityIndex);
    assert(componentData != nullptr);
    return *componentData;
}

template <ecs::IsComponent ComponentCls>
const ComponentCls& ecs::ArchetypedChunks::GetComponent(const chunkEntityIndex_t chunkEntityIndex) const
{
    const ComponentCls* componentData = TryGetComponent<ComponentCls>(chunkEntityIndex);
    assert(componentData != nullptr);
    return *componentData;
}

inline const ecs::Archetype& ecs::ArchetypedChunks::archetype() const { return m_archetype; }

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::chunkCount() const
{
    return static_cast<chunkEntityIndex_t>(m_chunksEntityCount.size());
}

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::aliveInChunk(const chunkEntityIndex_t chunkIndex) const
{
    assert(chunkIndex < m_chunksEntityCount.size());
    return m_chunksEntityCount[chunkIndex];
}

// =================================================== ChunkView ====================================================

template<ecs::IsComponent... ComponentCls>
ecs::ChunkView<ComponentCls...>::ChunkView(ArchetypedChunks* chunks, const chunkEntityIndex_t chunkIndex) :
    m_count(chunks->aliveInChunk(chunkIndex)),
    m_chunkIndex(chunkIndex)
{
    const chunkEntityIndex_t chunkBase = chunkIndex << MAX_ENTITIES_IN_CHUNK_BITS;
    m_componentArrays = std::tuple{
        std::bit_cast<ComponentCls*>(
            chunks->GetComponentData(chunkBase, ComponentRegistrator::GetComponentId<ComponentCls>())
        )...
    };
}

template <typename ValueType, ecs::IsComponent... ComponentCls>
auto ecs::ArchetypedChunks::begin()
{
    return iterator<ValueType, ComponentCls...>(this);
}

template <typename ValueType, ecs::IsComponent... ComponentCls>
auto ecs::ArchetypedChunks::end()
{
    return iterator<ValueType, ComponentCls...>();
}

inline ecs::chunkEntityIndex_t ecs::ArchetypedChunks::AllocateRawSlot(const entityId_t entityId)
{
    // - - - Find Chunk Index, Entity Index - - -
    chunkEntityIndex_t chunkIndex;
    chunkEntityIndex_t localEntityIndex = 0;

    if (const std::size_t minChunkIndexHasFreeEntities = m_hasFreeEntityInChunk.min();
        minChunkIndexHasFreeEntities == decltype(m_hasFreeEntityInChunk)::INVALID_INDEX)
    {
        chunkIndex = m_hasFreeEntityInChunk.size();

        m_localIndexToEntityId.push_back({});
        m_chunksEntityCount.push_back(1);
        m_hasFreeEntityInChunk.set(chunkIndex);
    }
    else
    {
        localEntityIndex = m_chunksEntityCount[minChunkIndexHasFreeEntities]++;
        chunkIndex = minChunkIndexHasFreeEntities;

        if (m_chunksEntityCount[minChunkIndexHasFreeEntities] == MAX_ENTITIES_IN_CHUNK)
            m_hasFreeEntityInChunk.reset(minChunkIndexHasFreeEntities);
    }
    assert(m_localIndexToEntityId[chunkIndex][localEntityIndex] == INVALID_ENTITY_ID);

    // - - - Ensure Backing Chunk Memory - - -

    for (const componentId_t componentId : m_archetype)
    {
        componentChunks_t& componentChunks = m_chunksByComponentId[componentId];

        if (componentChunks.size() <= chunkIndex)
        {
            const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);
            componentChunks.reserve(chunkIndex * 2);
            componentChunks.resize(chunkIndex + 1);
            componentChunks[chunkIndex] = std::make_unique<byte[]>(componentInfo.componentSize * MAX_ENTITIES_IN_CHUNK);
        }
    }

    m_localIndexToEntityId[chunkIndex][localEntityIndex] = entityId;
    return localEntityIndex + (chunkIndex << MAX_ENTITIES_IN_CHUNK_BITS);
}

template<ecs::PrefabEntityRef PrefabRef>
ecs::chunkEntityIndex_t ecs::ArchetypedChunks::Create(PrefabRef&& entity, const entityId_t entityId)
{
    const chunkEntityIndex_t chunkEntityIndex = AllocateRawSlot(entityId);
    const std::size_t chunkIndex = getChunkByEntityIndex(chunkEntityIndex);
    const std::size_t localEntityIndex = getLocalEntityIndex(chunkEntityIndex);

    const PrefabEntity::componentsData_t& componentsData = entity.GetComponentsData();
    for (const componentId_t componentId : m_archetype)
    {
        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);

        assert(componentsData[componentId] != nullptr);

        byte* dest = &m_chunksByComponentId[componentId][chunkIndex][componentInfo.componentSize * localEntityIndex];
        byte* src  = componentsData[componentId].get();

        if constexpr (!std::is_lvalue_reference_v<PrefabRef>)
            componentInfo.move(dest, src);
        else
            componentInfo.copy(dest, src);
    }

    return chunkEntityIndex;
}

