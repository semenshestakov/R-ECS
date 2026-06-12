#pragma once
#include <cassert>
#include "ecs/entities/EntitiesArchetypeStorage.hpp"
#include "ecs/entities/PrefabEntity.hpp"

// ======================================== EntitiesArchetypeStorage::iterator =========================================


template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::iterator(EntitiesArchetypeStorage* storage) :
    m_storage(storage),
    m_archetypeIndex(0)
{
    assert(storage != nullptr);

    if (storage->m_storageByArchetypeIndex.empty())
    {
        m_archetypeIndex = INVALID_ARCHETYPE_INDEX;
        return;
    }

    if (
        !s_archetype.isSubsetOf(storage->m_storageByArchetypeIndex[m_archetypeIndex].archetype()) or
        !storage->m_storageByArchetypeIndex[m_archetypeIndex].template begin<ValueType, ComponentCls...>()
        )
        advance();
    else
        m_archetypedChunksIt = m_storage->m_storageByArchetypeIndex[m_archetypeIndex].template begin<ValueType, ComponentCls...>();

}

template<typename ValueType, ecs::IsComponent... ComponentCls>
typename ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::value_type ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator*() const
{
    return *m_archetypedChunksIt;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>& ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator++()
{
    advance();
    return *this;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...> ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator++(int)
{
    iterator copy = *this;
    advance();
    return copy;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator==(const iterator& other) const
{
    return m_archetypedChunksIt == other.m_archetypedChunksIt && m_archetypeIndex == other.m_archetypeIndex;
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
bool ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator!=(const iterator& other) const
{
    return !(*this == other);
}


template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::advance()
{
    assert(m_storage != nullptr);
    assert(m_archetypeIndex != INVALID_ARCHETYPE_INDEX);

    if (m_archetypedChunksIt)
        ++m_archetypedChunksIt;

    if (!m_archetypedChunksIt)
    {
        while (++m_archetypeIndex < m_storage->m_storageByArchetypeIndex.size())
        {
            if (
                auto& chunks = m_storage->m_storageByArchetypeIndex[m_archetypeIndex];
                s_archetype.isSubsetOf(chunks.archetype())
                )
            {
                m_archetypedChunksIt = chunks.template begin<ValueType, ComponentCls...>();
                if (m_archetypedChunksIt)
                    return;
            }
        }

        // end state
        m_archetypeIndex = INVALID_ARCHETYPE_INDEX;
        m_archetypedChunksIt = {};
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


inline ecs::archetypeIndex_t ecs::EntitiesArchetypeStorage::findOrCreateArchetype(const Archetype& archetype)
{
    if (const auto it = m_archetypeIndexByHash.find(archetype.hash()); it != m_archetypeIndexByHash.end())
        return it->second;

    assert(m_archetypeIndexByHash.size() == m_storageByArchetypeIndex.size());
    const archetypeIndex_t archetypeIndex = m_archetypeIndexByHash.size();

    m_archetypeIndexByHash[archetype.hash()] = archetypeIndex;
    m_storageByArchetypeIndex.emplace_back(archetype);
    return archetypeIndex;
}

inline const ecs::Archetype& ecs::EntitiesArchetypeStorage::getArchetype(const archetypeIndex_t archetypeIndex) const
{
    assert(archetypeIndex < m_storageByArchetypeIndex.size());
    return m_storageByArchetypeIndex[archetypeIndex].archetype();
}

template<ecs::PrefabEntityRef PrefabRef>
ecs::ArchetypedChunkEntityLocation ecs::EntitiesArchetypeStorage::Create(PrefabRef&& prefabEntity, const entityId_t entityId)
{
    const Archetype& archetype = prefabEntity.getArchetype();
    const archetypeIndex_t archetypeIndex = findOrCreateArchetype(archetype);

    return {
        .archetypeIndex=archetypeIndex,
        .chunkEntityIndex=m_storageByArchetypeIndex[archetypeIndex].Create(std::forward<PrefabRef>(prefabEntity), entityId)
    };
}

inline ecs::EntitiesArchetypeStorage::EntityMigration ecs::EntitiesArchetypeStorage::MigrateEntity(
    const ArchetypedChunkEntityLocation& oldLocation, const Archetype& newArchetype, const entityId_t entityId
    )
{
    const archetypeIndex_t newArchetypeIndex = findOrCreateArchetype(newArchetype);

    const ArchetypedChunkEntityLocation newLocation {
        .archetypeIndex=newArchetypeIndex,
        .chunkEntityIndex=m_storageByArchetypeIndex[newArchetypeIndex].AllocateRawSlot(entityId)
    };

    // Move only the components shared by both archetypes. Components dropped by the
    // migration stay in the old slot (destructed by Destroy below); components added by
    // the migration are left raw for the caller to construct.
    const Archetype& oldArchetype = m_storageByArchetypeIndex[oldLocation.archetypeIndex].archetype();
    for (const componentId_t componentId : newArchetype)
    {
        if (!oldArchetype.test(componentId))
            continue;

        const RegisterComponentInfo& componentInfo = ComponentRegistrator::GetInfo(componentId);
        componentInfo.move(
            /* to */    GetComponentData(newLocation, componentId),
            /* from */  GetComponentData(oldLocation, componentId)
        );
    }

    const entityId_t swapRemovedEntityId = m_storageByArchetypeIndex[oldLocation.archetypeIndex].Destroy(oldLocation.chunkEntityIndex);

    return {.newLocation=newLocation, .swapRemovedEntityId=swapRemovedEntityId};
}

inline ecs::entityId_t ecs::EntitiesArchetypeStorage::Destroy(const ArchetypedChunkEntityLocation& entityLocation)
{
    return m_storageByArchetypeIndex.at(entityLocation.archetypeIndex).Destroy(entityLocation.chunkEntityIndex);
}

inline ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId)
{
    assert(entityLocation.archetypeIndex < m_storageByArchetypeIndex.size());
    return m_storageByArchetypeIndex[entityLocation.archetypeIndex].GetComponentData(entityLocation.chunkEntityIndex, componentId);
}

inline const ecs::byte* ecs::EntitiesArchetypeStorage::GetComponentData(const ArchetypedChunkEntityLocation& entityLocation, const componentId_t componentId) const
{
    assert(entityLocation.archetypeIndex < m_storageByArchetypeIndex.size());
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
