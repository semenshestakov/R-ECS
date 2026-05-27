#pragma once
#include "../EntitiesManager.hpp"


inline ecs::EntitiesManager::EntitiesManager() { resize(256); }

inline ecs::EntitiesManager::~EntitiesManager()
{
    for(entityId_t entityId = INVALID_ENTITY_ID + 1; entityId < m_lastEntityId; ++entityId)
    {
        if(m_versionByEntityIndex[entityId] != INVALID_ENTITY_VERSION)
            m_storage.Destroy(m_entitiesLocationByEntityIndex[entityId]);
    }
}

template<ecs::ReturnEntityConcept ReturnType>
inline ReturnType ecs::EntitiesManager::Create(const PrefabEntity& prefabEntity)
{
    Entity entity{};

    if(!m_freeEntities.empty())
    {
        entity = m_freeEntities.front();
        if(entity.version == MAX_ENTITY_VERSION)
            entity.version = INVALID_ENTITY_VERSION + 1; ///< reuse first entity.version
        else
            ++entity.version; ///< inc free entity

        m_freeEntities.pop();
    } else
    {
        entity.id = m_lastEntityId++;
        assert(entity.id != MAX_ENTITY_ID and entity.id != INVALID_ENTITY_ID);

        entity.version = INVALID_ENTITY_VERSION + 1;
        if(entity.id >= m_versionByEntityIndex.size())
        {
            resize(std::max<std::size_t>(m_versionByEntityIndex.size(), 1) * 2);
        }
    }

    assert(entity.id < m_lastEntityId);
    m_entitiesLocationByEntityIndex[entity.id] = m_storage.Create(prefabEntity);
    m_versionByEntityIndex[entity.id] = entity.version;
    ++m_isAliveEntitiesCount;

    if constexpr (std::is_same_v<ReturnType, EntityWrapper>)
        return {entity, *this};
    else
        return entity;
}

template<ecs::ReturnEntityConcept ReturnType>
ReturnType ecs::EntitiesManager::Create(PrefabEntity&& prefabEntity)
{
    return Create<ReturnType>(prefabEntity);
}

inline void ecs::EntitiesManager::Destroy(const Entity& entity)
{
    if(!IsAlive(entity))
        return;

    assert(entity.id < m_lastEntityId);
    assert(m_versionByEntityIndex.size() == m_entitiesLocationByEntityIndex.size());

    m_storage.Destroy(m_entitiesLocationByEntityIndex[entity.id]);
    m_versionByEntityIndex[entity.id] = {};
    m_freeEntities.emplace(entity);
    --m_isAliveEntitiesCount;
}

inline void ecs::EntitiesManager::Destroy(const EntityWrapper& entity) { return Destroy(entity.getEntity()); }

inline bool ecs::EntitiesManager::IsAlive(const Entity& entity) const
{
    if(entity.id >= m_versionByEntityIndex.size())
        return false;

    if(m_versionByEntityIndex[entity.id] != entity.version)
        return false;

    return true;
}

inline bool ecs::EntitiesManager::IsAlive(const EntityWrapper& entity) const { return IsAlive(entity.getEntity()); }

template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity)
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity) const
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity)
{
    return reinterpret_cast<ComponentCls*>(
            GetComponentData(entity, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity) const
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

inline ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId)
{
    if(!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

inline const ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId) const
{
    if(!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesManager::view()
{
    return m_storage.begin<ComponentCls...>();
}

inline void ecs::EntitiesManager::resize(const std::size_t size)
{
    m_versionByEntityIndex.resize(size);
    m_entitiesLocationByEntityIndex.resize(size);
}
