#include <cassert>
#include "ecs/entities/EntitiesManager.hpp"

#include "ecs/entities/PrefabEntity.hpp"


ecs::EntitiesManager::EntitiesManager()
{
    resize(256);
}

ecs::EntitiesManager::~EntitiesManager()
{
    for (entityId_t entityId = INVALID_ENTITY_ID + 1; entityId < m_lastEntityId; ++entityId)
    {
        if (m_versionByEntityIndex[entityId] != INVALID_ENTITY_VERSION)
            m_storage.Destroy(m_entitiesLocationByEntityIndex[entityId]);
    }
}

ecs::EntityWrapper ecs::EntitiesManager::Create(PrefabEntity& prefabEntity)
{
    Entity entity {};

    if(!m_freeEntities.empty())
    {
        entity = m_freeEntities.front();
        if (entity.version == MAX_ENTITY_VERSION)
            entity.version = INVALID_ENTITY_VERSION + 1;        ///< reuse first entity.version
        else
            ++entity.version;                                   ///< inc free entity

        m_freeEntities.pop();
    }
    else
    {
        entity.id = m_lastEntityId++;
        assert(entity.id != MAX_ENTITY_ID and entity.id != INVALID_ENTITY_ID);

        entity.version = INVALID_ENTITY_VERSION + 1;
        if (entity.id >= m_versionByEntityIndex.size())
        {
            resize(std::max<std::size_t>(m_versionByEntityIndex.size(), 1) * 2);
        }
    }

    assert(entity.id < m_lastEntityId);
    m_entitiesLocationByEntityIndex[entity.id] = m_storage.Create(prefabEntity);
    m_versionByEntityIndex[entity.id] = entity.version;
    ++m_isAliveEntitiesCount;

    return {entity, *this};
}

ecs::EntityWrapper ecs::EntitiesManager::Create(PrefabEntity&& prefabEntity)
{
    return Create(prefabEntity);
}

void ecs::EntitiesManager::Destroy(const Entity& entity)
{
    if (!IsAlive(entity))
        return;

    assert(entity.id < m_lastEntityId);
    assert(m_versionByEntityIndex.size() == m_entitiesLocationByEntityIndex.size());

    m_storage.Destroy(m_entitiesLocationByEntityIndex[entity.id]);
    m_versionByEntityIndex[entity.id] = {};
    m_freeEntities.emplace(entity);
    --m_isAliveEntitiesCount;
}

void ecs::EntitiesManager::Destroy(const EntityWrapper& entity)
{
    return Destroy(entity.getEntity());
}

bool ecs::EntitiesManager::IsAlive(const Entity& entity) const
{
    if (entity.id >= m_versionByEntityIndex.size())
        return false;

    if (m_versionByEntityIndex[entity.id] != entity.version)
        return false;

    return true;
}

bool ecs::EntitiesManager::IsAlive(const EntityWrapper& entity) const
{
    return IsAlive(entity.getEntity());
}

ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId)
{
    if (!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

const ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId) const
{
    if (!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

void ecs::EntitiesManager::resize(const std::size_t size)
{
    m_versionByEntityIndex.resize(size);
    m_entitiesLocationByEntityIndex.resize(size);
}
