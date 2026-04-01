#pragma once
#include "../EntitiesManager.hpp"


template<ecs::DerivedComponent ComponentCls>
ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity)
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::DerivedComponent ComponentCls>
const ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity) const
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::DerivedComponent ComponentCls>
ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity)
{
    return reinterpret_cast<ComponentCls*>(GetComponentData(entity, ComponentCls::componentId));
}

template<ecs::DerivedComponent ComponentCls>
const ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity) const
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity) assert(component != nullptr);
    return *component;
}

template<ecs::DerivedComponent... ComponentCls>
auto ecs::EntitiesManager::view()
{
    return m_storage.begin<ComponentCls...>();
}
