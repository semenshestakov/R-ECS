#pragma once
#include "../EntityWrapper.hpp"
#include "../EntitiesManager.hpp"

inline ecs::EntityWrapper::EntityWrapper(const Entity& entity, EntitiesManager& entitiesManager) :
    m_entity(entity),
    m_managerRef(entitiesManager)
{}

inline bool ecs::EntityWrapper::IsAlive() const
{
    return m_managerRef.get().IsAlive(m_entity);
}

inline void ecs::EntityWrapper::SelfDestroy() const
{
    return m_managerRef.get().Destroy(m_entity);
}

inline ecs::byte* ecs::EntityWrapper::GetComponentData(const componentId_t componentId)
{
    return m_managerRef.get().GetComponentData(m_entity, componentId);
}

inline const ecs::byte* ecs::EntityWrapper::GetComponentData(const componentId_t componentId) const
{
    return m_managerRef.get().GetComponentData(m_entity, componentId);
}
