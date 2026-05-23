#pragma once
#include "../EntitiesManager.hpp"


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
    return reinterpret_cast<ComponentCls*>(GetComponentData(entity, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity) const
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesManager::view()
{
    return m_storage.begin<ComponentCls...>();
}
