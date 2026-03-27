#pragma once
#include <cassert>
#include "../EntityWrapper.hpp"


template<ecs::DerivedComponent ComponentCls>
ComponentCls& ecs::EntityWrapper::GetComponent()
{
    ComponentCls* componentPtr = TryGetComponent<ComponentCls>();
    assert(componentPtr != nullptr);
    return *componentPtr;
}

template<ecs::DerivedComponent ComponentCls>
const ComponentCls& ecs::EntityWrapper::GetComponent() const
{
    const ComponentCls* componentPtr = TryGetComponent<ComponentCls>();
    assert(componentPtr != nullptr);
    return *componentPtr;
}

template<ecs::DerivedComponent ComponentCls>
ComponentCls* ecs::EntityWrapper::TryGetComponent()
{
    return reinterpret_cast<ComponentCls*>(GetComponentData(ComponentCls::componentId));
}

template<ecs::DerivedComponent ComponentCls>
const ComponentCls* ecs::EntityWrapper::TryGetComponent() const
{
    return reinterpret_cast<const ComponentCls*>(GetComponentData(ComponentCls::componentId));
}
