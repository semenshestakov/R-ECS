#pragma once
#include "../EntitiesManager.hpp"


namespace ecs
{

    template<DerivedComponent ComponentCls>
    ComponentCls& EntitiesManager::GetComponent(const Entity& entity)
    {
        ComponentCls* component = TryGetComponent<ComponentCls>(entity);
        assert(component != nullptr);
        return *component;
    }

    template<DerivedComponent ComponentCls>
    const ComponentCls& EntitiesManager::GetComponent(const Entity& entity) const
    {
        ComponentCls* component = TryGetComponent<ComponentCls>(entity);
        assert(component != nullptr);
        return *component;
    }

    template<DerivedComponent ComponentCls>
    ComponentCls* EntitiesManager::TryGetComponent(const Entity& entity)
    {
        return reinterpret_cast<ComponentCls*>(GetComponentData(entity, ComponentCls::componentId));
    }

    template<DerivedComponent ComponentCls>
    const ComponentCls* EntitiesManager::TryGetComponent(const Entity& entity) const
    {
        ComponentCls* component = TryGetComponent<ComponentCls>(entity) assert(component != nullptr);
        return *component;
    }

    template<DerivedComponent... ComponentCls>
    auto EntitiesManager::view()
    {
        return m_storage.begin<ComponentCls...>();
    }

}
