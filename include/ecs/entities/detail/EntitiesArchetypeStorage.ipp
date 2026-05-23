#pragma once
#include "../EntitiesArchetypeStorage.hpp"


// ====================================== EntitiesArchetypeStorage::iterator ======================================

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::iterator(EntitiesArchetypeStorage* storage, const bool isEnd) :
    m_storage(storage)
{
    if(isEnd || !m_storage)
    {
        m_isEnded = true;
        return;
    }

    m_currentArchetype = 0;
    m_entityLocation.chunkEntityIndex = 0;
    advanceToNextValid();
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
typename ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::value_type ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::operator*() const
{
    if constexpr(std::is_same_v<std::tuple<ComponentCls&...>, value_type>)
    {
        return std::forward_as_tuple(m_storage->GetComponent<ComponentCls>(m_entityLocation)...);
    }
    else if constexpr(std::is_same_v<ArchetypedChunkEntityLocation, value_type>)
    {
        return m_entityLocation;
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
    if(m_isEnded && other.m_isEnded)
        return true;

    return m_storage == other.m_storage && m_isEnded == other.m_isEnded &&
           m_currentArchetype == other.m_currentArchetype && m_entityLocation == other.m_entityLocation;
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
    return iterator<ValueType, ComponentCls...>(m_storage, false);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>
ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::end() const
{
    return iterator<ValueType, ComponentCls...>(m_storage, true);
}

template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::advance()
{
    if(m_isEnded)
        return;

    ++m_entityLocation.chunkEntityIndex;
    advanceToNextValid();
}
template<typename ValueType, ecs::IsComponent... ComponentCls>
void ecs::EntitiesArchetypeStorage::iterator<ValueType, ComponentCls...>::advanceToNextValid()
{
    const auto& storage = *m_storage;

    while(true)
    {
        if(m_currentArchetype >= storage.m_storageByArchetypeIndex.size())
        {
            m_isEnded = true;
            return;
        }

        const auto& chunks = storage.m_storageByArchetypeIndex[m_currentArchetype];
        if(!chunks.archetype().isSubsetOf(Archetype::GetArchetype<ComponentCls...>()))
        {
            ++m_currentArchetype;
            m_entityLocation.chunkEntityIndex = 0;
            continue;
        }

        const auto limit = chunks.getLastChunkEntityIndex();
        while(m_entityLocation.chunkEntityIndex < limit)
        {
            if(chunks.IsAlive(m_entityLocation.chunkEntityIndex))
            {
                m_entityLocation.archetypeIndex = m_currentArchetype;
                return;
            }

            ++m_entityLocation.chunkEntityIndex;
        }

        ++m_currentArchetype;
        m_entityLocation.chunkEntityIndex = 0;
    }
}

// ===================================== EntitiesArchetypeStorage::begin | end =====================================

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesArchetypeStorage::begin()
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>(this, false);
}

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesArchetypeStorage::end()
{
    return iterator<iter_value_type<ComponentCls...>, ComponentCls...>(this, true);
}

// ======================================= EntitiesArchetypeStorage::methods =======================================

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location)
{
    return reinterpret_cast<ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesArchetypeStorage::TryGetComponent(const ArchetypedChunkEntityLocation& location) const
{
    return reinterpret_cast<const ComponentCls*>(GetComponentData(location, ComponentRegistrator::GetСomponentId<ComponentCls>()));
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
