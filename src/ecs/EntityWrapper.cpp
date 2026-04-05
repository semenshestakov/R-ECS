#include "ecs/entities/EntityWrapper.hpp"
#include "ecs/entities/EntitiesManager.hpp"


ecs::EntityWrapper::EntityWrapper(const Entity& entity, EntitiesManager& entitiesManager) :
    m_entity(entity),
    m_managerRef(entitiesManager)
{
}

bool ecs::EntityWrapper::IsAlive() const
{
    return m_managerRef.get().IsAlive(m_entity);
}

void ecs::EntityWrapper::SelfDestroy() const
{
    return m_managerRef.get().Destroy(m_entity);
}

ecs::byte* ecs::EntityWrapper::GetComponentData(const componentId_t componentId)
{
    return m_managerRef.get().GetComponentData(m_entity, componentId);
}

const ecs::byte* ecs::EntityWrapper::GetComponentData(const componentId_t componentId) const
{
    return m_managerRef.get().GetComponentData(m_entity, componentId);
}

